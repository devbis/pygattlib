// -*- mode: c++; coding: utf-8; tab-width: 4 -*-

// Copyright (C) 2014, Oscar Acena <oscaracena@gmail.com>
// This software is under the terms of Apache License v2 or later.

#ifndef _GATTSERVICES_H_
#define _GATTSERVICES_H_

#include <boost/python/dict.hpp>
#include <map>

#define EIR_NAME_SHORT     0x08  /* shortened local name */
#define EIR_NAME_COMPLETE  0x09  /* complete local name */

#define BLE_EVENT_TYPE     0x05
#define BLE_SCAN_RESPONSE  0x04

enum ScanEntryType {
    FLAGS                     = 0x01,
    INCOMPLETE_16B_SERVICES   = 0x02,
    COMPLETE_16B_SERVICES     = 0x03,
    INCOMPLETE_32B_SERVICES   = 0x04,
    COMPLETE_32B_SERVICES     = 0x05,
    INCOMPLETE_128B_SERVICES  = 0x06,
    COMPLETE_128B_SERVICES    = 0x07,
    SHORT_LOCAL_NAME          = 0x08,
    COMPLETE_LOCAL_NAME       = 0x09,
    TX_POWER                  = 0x0A,
    SERVICE_SOLICITATION_16B  = 0x14,
    SERVICE_SOLICITATION_32B  = 0x1F,
    SERVICE_SOLICITATION_128B = 0x15,
    SERVICE_DATA_16B          = 0x16,
    SERVICE_DATA_32B          = 0x20,
    SERVICE_DATA_128B         = 0x21,
    PUBLIC_TARGET_ADDRESS     = 0x17,
    RANDOM_TARGET_ADDRESS     = 0x18,
    APPEARANCE                = 0x19,
    ADVERTISING_INTERVAL      = 0x1A,
    MANUFACTURER              = 0xFF
};

class DiscoveryService {
public:
	DiscoveryService(const std::string device="hci0");
	virtual ~DiscoveryService();
	boost::python::dict discover(int timeout);
	boost::python::object set_callback(PyObject* callback);


protected:
	void enable_scan_mode();
	void get_advertisements(int timeout, boost::python::dict & ret);
	virtual void process_input(unsigned char* buffer, int size,
			boost::python::dict & ret);
	std::string parse_name(uint8_t* data, size_t size);
	void disable_scan_mode();

	std::string _device;
	int _device_desc;
	int _timeout;
	PyObject* _callback;
};

#endif // _GATTSERVICES_H_
