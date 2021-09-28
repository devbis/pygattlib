// -*- mode: c++; coding: utf-8; tab-width: 4 -*-

// Copyright (C) 2014, Oscar Acena <oscaracena@gmail.com>
// This software is under the terms of Apache License v2 or later.

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <exception>

#include "gattlib.h"
#include "gattservices.h"

DiscoveryService::DiscoveryService(const std::string device) :
	_device(device),
	_device_desc(-1),
	_is_running(false),
 	_callback(boost::python::object().ptr()) {

	int dev_id = hci_devid(device.c_str());
	if (dev_id < 0)
		throw std::runtime_error("Invalid device!");

	_device_desc = hci_open_dev(dev_id);
	if (_device_desc < 0)
		throw std::runtime_error("Could not open device!");
	}

DiscoveryService::~DiscoveryService() {
	if (_device_desc != -1)
		hci_close_dev(_device_desc);
}

void
DiscoveryService::enable_scan_mode() {
	int result;
	uint8_t scan_type = 0x01;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);
	uint8_t own_type = 0x00;
	uint8_t filter_policy = 0x00;

	result = hci_le_set_scan_parameters
		(_device_desc, scan_type, interval, window,
		 own_type, filter_policy, 10000);

	if (result < 0)
		throw std::runtime_error
			("Set scan parameters failed (are you root?)");

	result = hci_le_set_scan_enable(_device_desc, 0x01, 1, 10000);
	if (result < 0)
		throw std::runtime_error("Enable scan failed");
}

void
DiscoveryService::process_input(unsigned char* buffer, int size,
		boost::python::dict & ret) {
	unsigned char* ptr = buffer + HCI_EVENT_HDR_SIZE + 1;
	evt_le_meta_event* meta = (evt_le_meta_event*) ptr;

	if (meta->subevent != EVT_LE_ADVERTISING_REPORT)
		return;

	le_advertising_info* info;
	info = (le_advertising_info*) (meta->data + 1);

	char addr[18];
	ba2str(&info->bdaddr, addr);

	std::string name = parse_name(info->data, info->length);
	ret[addr] = name;
	if (_callback != Py_None) {
		boost::python::dict advert_ret;
		boost::python::dict advert_list;
		unsigned char *data = info->data;

		int offset = 0;
		while (offset < info->length) {
			uint8_t field_len = data[0];

			if (field_len == 0 || offset + field_len > size)
				break;
			advert_list[data[1]] = boost::python::handle<>(PyBytes_FromStringAndSize(
				(const char*) info->data + offset + 2,
				field_len - 1
			));

			offset += field_len + 1;
			data += field_len + 1;
		}
        int8_t rssi;
        rssi = data[info->length];
        if ((uint8_t) rssi == 0x99)
        	rssi = 127;

		advert_ret["name"] = name;
		advert_ret["rssi"] = rssi;
		advert_ret["info"] = advert_list;

 		boost::python::call<void>(
 			_callback,
 			addr,
 			advert_ret
		);
	}
}

std::string
DiscoveryService::parse_name(uint8_t* data, size_t size) {
	size_t offset = 0;
	std::string unknown = "";

	while (offset < size) {
		uint8_t field_len = data[0];
		size_t name_len;

		if (field_len == 0 || offset + field_len > size)
			return unknown;

		switch (data[1]) {
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			name_len = field_len - 1;
			if (name_len > size)
				return unknown;

			return std::string((const char*)(data + 2), name_len);
		}

		offset += field_len + 1;
		data += field_len + 1;
	}

	return unknown;
}

void
DiscoveryService::disable_scan_mode() {
	if (_device_desc == -1)
		throw std::runtime_error("Could not disable scan, not enabled yet");

	int result = hci_le_set_scan_enable(_device_desc, 0x00, 1, 10000);
	if (result < 0)
		throw std::runtime_error("Disable scan failed");
}

boost::python::dict
DiscoveryService::discover(int timeout) {
	boost::python::dict retval;
	start();
	_is_running = true;
	int ts = time(NULL);

	while(_is_running) {
		do_step();
		int elapsed = time(NULL) - ts;
		if (timeout && elapsed >= timeout)
			_is_running = false;
	}
	stop();

	return retval;
}

void
DiscoveryService::start() {
	enable_scan_mode();

	socklen_t slen = sizeof(_old_options);
	if (getsockopt(_device_desc, SOL_HCI, HCI_FILTER,
				   &_old_options, &slen) < 0)
		throw std::runtime_error("Could not get socket options");

	struct hci_filter new_options;
	hci_filter_clear(&new_options);
	hci_filter_set_ptype(HCI_EVENT_PKT, &new_options);
	hci_filter_set_event(EVT_LE_META_EVENT, &new_options);

	if (setsockopt(_device_desc, SOL_HCI, HCI_FILTER,
				   &new_options, sizeof(new_options)) < 0)
		throw std::runtime_error("Could not set socket options\n");

}

boost::python::dict
DiscoveryService::do_step() {
	int len;
	unsigned char buffer[HCI_MAX_EVENT_SIZE];
	boost::python::dict ret;

	FD_ZERO(&_read_set);
	FD_SET(_device_desc, &_read_set);

	struct timeval wait = (struct timeval){0};
	wait.tv_usec = 100000;  // wait by 0.1 sec chunks

	int err = select(FD_SETSIZE, &_read_set, NULL, NULL, &wait);
	if (err <= 0)
		return ret;

	len = read(_device_desc, buffer, sizeof(buffer));
	process_input(buffer, len, ret);
	return ret;
}

void
DiscoveryService::stop() {
	setsockopt(_device_desc, SOL_HCI, HCI_FILTER,
			   &_old_options, sizeof(_old_options));
	disable_scan_mode();
}

boost::python::object
DiscoveryService::set_callback(PyObject* callback) {
	_callback = callback;

	return boost::python::object();
}
