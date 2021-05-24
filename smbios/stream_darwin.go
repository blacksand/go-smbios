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

//+build darwin

package smbios

import "C"
import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"reflect"
	"unsafe"
)

/*
#cgo LDFLAGS: -framework CoreFoundation -framework IOKit
#include <stdint.h>
int getDMITable(uint8_t *buf, size_t bufLen);
int getEntryPoint(uint8_t *buf, size_t bufLen);
*/
import "C"

func stream() (io.ReadCloser, EntryPoint, error) {
	ep, err := getDarwinEntryPoint()
	if err != nil {
		return nil, nil, err
	}

	tb, err := getDarwinTable(ep)
	if err != nil {
		return nil, nil, err
	}

	return ioutil.NopCloser(bytes.NewReader(tb)), ep, nil
}

func getDarwinEntryPoint() (EntryPoint, error) {
	buf := make([]byte, 64)
	p := (*reflect.SliceHeader)(unsafe.Pointer(&buf))

	var result C.int = C.getEntryPoint((*C.uint8_t)(unsafe.Pointer(p.Data)), C.size_t(64))

	switch result {
	case -1:
		return nil, fmt.Errorf("AppleSMBIOS service is unreachable")
	case -2:
		return nil, fmt.Errorf("SMBIOS entry point is unreachable")
	}

	ep, err := ParseEntryPoint(ioutil.NopCloser(bytes.NewBuffer(buf)))
	if err != nil {
		return nil, err
	}

	return ep, nil
}

func getDarwinTable(ep EntryPoint) ([]byte, error) {
	_, size := ep.Table()
	buf := make([]byte, size)
	p := (*reflect.SliceHeader)(unsafe.Pointer(&buf))

	var result C.int = C.getDMITable((*C.uint8_t)(unsafe.Pointer(p.Data)), C.size_t(size))

	switch result {
	case -1:
		return nil, fmt.Errorf("AppleSMBIOS service is unreachable")
	case -2:
		return nil, fmt.Errorf("no data in AppleSMBIOS IOService")
	case -3:
		return nil, fmt.Errorf("SMBIOS property data is unreachable")
	}

	return buf, nil

}
