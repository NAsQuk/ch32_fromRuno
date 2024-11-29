#include"rvnettcp.h"
#include "memman.h"
#include "crc.h"

#define RVTCP_TID	0
#define RVTCP_PID	2
#define RVTCP_LEN	4
#define RVTCP_UID	6
#define RVTCP_FUNC	7
#define RVTCP_DATA	8

#define MB_TCP_PROTOCOL_ID  0

RVNET_DATATYPE RVnetTcpSlaveProcess(uint8 *buf, RVNET_DATATYPE pkSize,
		uint8 device_address) {
	uint16 len, pid;

	len = buf[RVTCP_LEN] << 8U;
	len |= buf[RVTCP_LEN + 1];

	pid = buf[RVTCP_PID] << 8U;
	pid |= buf[RVTCP_PID + 1];

	switch (buf[RVTCP_FUNC]) {
	case 0x00:
		len = ReadDeviceID(&buf[RVTCP_DATA]);
		break;
	case 0x01:
	case 0x02:
		len = ReadNBits(&buf[RVTCP_DATA]);
		break;
	case 0x03:
	case 0x04:
		len = ReadNWords(&buf[RVTCP_DATA]);
		break;
	case 0x05:
		len = WriteBit(&buf[RVTCP_DATA]);
		break;
	case 0x10:
		//pkSize = WriteNWords(&buf[RVTCP_DATA]);
		len = WriteNWords(&buf[RVTCP_DATA]);
		break;
	default:
		len = ErrorAddress(&buf[RVTCP_DATA]);
		break;
	}
	len += 1;
	buf[RVTCP_LEN] = (len + 1) >> 8U;
	buf[RVTCP_LEN + 1] = (len + 1) & 0xFF;
	return (len + RVTCP_FUNC);
}

