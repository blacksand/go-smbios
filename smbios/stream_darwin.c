// Copyright 2017-2018 DigitalOcean.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

int getEntryPoint(uint8_t *buf, size_t bufLen) {
	CFDataRef dataRef;
	mach_port_t masterPort;
	io_service_t service = kIOMasterPortDefault;

	IOMasterPort(kIOMasterPortDefault, &masterPort);
	service = IOServiceGetMatchingService(masterPort, IOServiceMatching("AppleSMBIOS"));
	if (service == kIOMasterPortDefault) {
		// "AppleSMBIOS service is unreachable, sorry."
		return -1;
	}

	dataRef = (CFDataRef) IORegistryEntryCreateCFProperty(service, CFSTR("SMBIOS-EPS"), kCFAllocatorDefault, kNilOptions);
	if (dataRef == NULL) {
		// "SMBIOS entry point is unreachable, sorry."
		return -2;
	}

	size_t len = CFDataGetLength(dataRef) * sizeof(uint8_t);
	len = len > bufLen ? bufLen : len;

	CFDataGetBytes(dataRef, CFRangeMake(0, len), (UInt8*)buf);
	CFRelease(dataRef);

	IOObjectRelease(service);

	return len;
}

int getDMITable(uint8_t *buf, size_t bufLen) {
	CFDataRef dataRef;
	mach_port_t masterPort;
	io_service_t service = MACH_PORT_NULL;
	CFMutableDictionaryRef properties = NULL;

	IOMasterPort(MACH_PORT_NULL, &masterPort);

	service = IOServiceGetMatchingService(masterPort, IOServiceMatching("AppleSMBIOS"));
	if (service == MACH_PORT_NULL) {
		return -1;
	}

	if (kIOReturnSuccess != IORegistryEntryCreateCFProperties(service, &properties, kCFAllocatorDefault, kNilOptions)) {
		IOObjectRelease(service);
		return -2;
	}

	if (!CFDictionaryGetValueIfPresent(properties, CFSTR( "SMBIOS"), (const void **)&dataRef)) {
		if (NULL != properties) {
			CFRelease(properties);
		}
		IOObjectRelease(service);
		return -3;
	}

	size_t len = CFDataGetLength(dataRef);
	len = len > bufLen ? bufLen : len;

	CFDataGetBytes(dataRef, CFRangeMake(0, len), (UInt8*)buf);

	// See: The Get Rule
	// https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFMemoryMgmt/Concepts/Ownership.html#//apple_ref/doc/uid/20001148-SW1
	//	if (NULL != dataRef) {
	//		CFRelease(dataRef);
	//	}

	/*
	 * This CFRelease throws 'Segmentation fault: 11' since macOS 10.12, if
	 * the compiled binary is not signed with an Apple developer profile.
	 */
	if (NULL != properties) {
		CFRelease(properties);
	}

	IOObjectRelease(service);

	return len;
}
