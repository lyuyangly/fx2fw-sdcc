#ifndef USB_REQUESTS_H
#define USB_REQUESTS_H

// Standard USB requests.
// These are contained in end point 0 setup packets
// format of bmRequestType byte

#define	bmRT_DIR_MASK		(0x1 << 7)
#define	bmRT_DIR_IN         (1 << 7)
#define	bmRT_DIR_OUT		(0 << 7)

#define	bmRT_TYPE_MASK		(0x3 << 5)
#define	bmRT_TYPE_STD		(0 << 5)
#define	bmRT_TYPE_CLASS		(1 << 5)
#define	bmRT_TYPE_VENDOR	(2 << 5)
#define	bmRT_TYPE_RESERVED	(3 << 5)

#define	bmRT_RECIP_MASK		(0x1f << 0)
#define	bmRT_RECIP_DEVICE	(0 << 0)
#define	bmRT_RECIP_INTERFACE	(1 << 0)
#define	bmRT_RECIP_ENDPOINT	(2 << 0)
#define	bmRT_RECIP_OTHER	(3 << 0)


// standard request codes (bRequest)

#define	RQ_GET_STATUS		0
#define	RQ_CLEAR_FEATURE	1
#define	RQ_RESERVED_2		2
#define	RQ_SET_FEATURE		3
#define	RQ_RESERVED_4		4
#define	RQ_SET_ADDRESS		5
#define	RQ_GET_DESCR		6
#define	RQ_SET_DESCR		7
#define	RQ_GET_CONFIG		8
#define	RQ_SET_CONFIG		9
#define	RQ_GET_INTERFACE       10
#define	RQ_SET_INTERFACE       11
#define	RQ_SYNCH_FRAME	       12

// standard descriptor types

#define	DT_DEVICE		1
#define	DT_CONFIG		2
#define	DT_STRING		3
#define	DT_INTERFACE		4
#define	DT_ENDPOINT		5
#define	DT_DEVQUAL		6
#define	DT_OTHER_SPEED		7
#define	DT_INTERFACE_POWER	8

// standard feature selectors

#define	FS_ENDPOINT_HALT	0	// recip: endpoint
#define	FS_DEV_REMOTE_WAKEUP	1	// recip: device
#define	FS_TEST_MODE		2	// recip: device

// Get Status device attributes

#define	bmGSDA_SELF_POWERED	0x01
#define	bmGSDA_REM_WAKEUP	0x02


#endif
