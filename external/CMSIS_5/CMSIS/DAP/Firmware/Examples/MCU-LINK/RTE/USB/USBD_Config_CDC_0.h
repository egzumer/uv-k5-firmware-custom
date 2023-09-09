/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Device
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    USBD_Config_CDC_0.h
 * Purpose: USB Device Communication Device Class (CDC) Configuration
 * Rev.:    V5.2.0
 *----------------------------------------------------------------------------*/

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>USB Device: Communication Device Class (CDC) 0
//   <o>Assign Device Class to USB Device # <0-3>
//   <i>Select USB Device that is used for this Device Class instance
#define USBD_CDC0_DEV                    0

//   <o>Communication Class Subclass
//   <i>Specifies the model used by the CDC class.
//     <2=>Abstract Control Model (ACM)
//     <13=>Network Control Model (NCM)
#define USBD_CDC0_SUBCLASS               2

//   <o>Communication Class Protocol
//   <i>Specifies the protocol used by the CDC class.
//     <0=>No protocol (Virtual COM)
//     <255=>Vendor-specific (RNDIS)
#define USBD_CDC0_PROTOCOL               0

//   <h>Interrupt Endpoint Settings
//   <i>By default, the settings match the first USB Class instance in a USB Device.
//   <i>Endpoint conflicts are flagged by compile-time error messages.

//     <o.0..3>Interrupt IN Endpoint Number
//               <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
//       <8=>8   <9=>9   <10=>10 <11=>11 <12=>12 <13=>13 <14=>14 <15=>15
#define USBD_CDC0_EP_INT_IN              3


//     <h>Endpoint Settings
//       <i>Parameters are used to create Endpoint Descriptors
//       <i>and for memory allocation in the USB component.

//       <h>Full/Low-speed (High-speed disabled)
//       <i>Parameters apply when High-speed is disabled in USBD_Config_n.c
//         <o.0..6>Maximum Endpoint Packet Size (in bytes) <0-64>
//         <i>Specifies the physical packet size used for information exchange.
//         <i>Maximum value is 64.
#define USBD_CDC0_WMAXPACKETSIZE         16

//         <o.0..7>Endpoint polling Interval (in ms) <1-255>
//         <i>Specifies the frequency of requests initiated by USB Host for
//         <i>getting the notification.
#define USBD_CDC0_BINTERVAL              2

//       </h>

//       <h>High-speed
//       <i>Parameters apply when High-speed is enabled in USBD_Config_n.c
//
//         <o.0..10>Maximum Endpoint Packet Size (in bytes) <0-1024>
//         <i>Specifies the physical packet size used for information exchange.
//         <i>Maximum value is 1024.
//         <o.11..12>Additional transactions per microframe
//         <i>Additional transactions improve communication performance.
//           <0=>None <1=>1 additional <2=>2 additional
#define USBD_CDC0_HS_WMAXPACKETSIZE      16

//         <o.0..4>Endpoint polling Interval (in 125 us intervals)
//         <i>Specifies the frequency of requests initiated by USB Host for
//         <i>getting the notification.
//           <1=>    1 <2=>    2 <3=>     4 <4=>     8
//           <5=>   16 <6=>   32 <7=>    64 <8=>   128
//           <9=>  256 <10=> 512 <11=> 1024 <12=> 2048
//           <13=>4096 <14=>8192 <15=>16384 <16=>32768
#define USBD_CDC0_HS_BINTERVAL           2

//       </h>
//     </h>
//   </h>


//   <h>Bulk Endpoint Settings
//   <i>By default, the settings match the first USB Class instance in a USB Device.
//   <i>Endpoint conflicts are flagged by compile-time error messages.

//     <o.0..3>Bulk IN Endpoint Number
//               <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
//       <8=>8   <9=>9   <10=>10 <11=>11 <12=>12 <13=>13 <14=>14 <15=>15
#define USBD_CDC0_EP_BULK_IN             4

//     <o.0..3>Bulk OUT Endpoint Number
//               <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
//       <8=>8   <9=>9   <10=>10 <11=>11 <12=>12 <13=>13 <14=>14 <15=>15
#define USBD_CDC0_EP_BULK_OUT            4


//     <h>Endpoint Settings
//       <i>Parameters are used to create USB Descriptors and for memory
//       <i>allocation in the USB component.
//
//       <h>Full/Low-speed (High-speed disabled)
//       <i>Parameters apply when High-speed is disabled in USBD_Config_n.c
//         <o.0..6>Maximum Endpoint Packet Size (in bytes) <8=>8 <16=>16 <32=>32 <64=>64
//         <i>Specifies the physical packet size used for information exchange.
//         <i>Maximum value is 64.
#define USBD_CDC0_WMAXPACKETSIZE1        64

//       </h>

//       <h>High-speed
//       <i>Parameters apply when High-speed is enabled in USBD_Config_n.c
//
//         <o.0..9>Maximum Endpoint Packet Size (in bytes) <512=>512
//         <i>Specifies the physical packet size used for information exchange.
//         <i>Only available value is 512.
#define USBD_CDC0_HS_WMAXPACKETSIZE1     512

//         <o.0..7>Maximum NAK Rate <0-255>
//         <i>Specifies the interval in which Bulk Endpoint can NAK.
//         <i>Value of 0 indicates that Bulk Endpoint never NAKs.
#define USBD_CDC0_HS_BINTERVAL1          0

//       </h>
//     </h>
//   </h>

//   <h>Communication Device Class Settings
//   <i>Parameters are used to create USB Descriptors and for memory allocation
//   <i>in the USB component.
//
//     <s.126>Communication Class Interface String
#define USBD_CDC0_CIF_STR_DESC           L"USB_CDC0_0"

//     <s.126>Data Class Interface String
#define USBD_CDC0_DIF_STR_DESC           L"USB_CDC0_1"

//     <h>Abstract Control Model Settings

//       <h>Call Management Capabilities
//       <i>Specifies which call management functionality is supported.
//         <o.1>Call Management channel
//           <0=>Communication Class Interface only
//           <1=>Communication and Data Class Interface
//         <o.0>Device Call Management handling
//           <0=>None
//           <1=>All
//       </h>
#define USBD_CDC0_ACM_CM_BM_CAPABILITIES 0x03

//       <h>Abstract Control Management Capabilities
//       <i>Specifies which abstract control management functionality is supported.
//         <o.3>D3 bit
//           <i>Enabled = Supports the notification Network_Connection
//         <o.2>D2 bit
//           <i>Enabled = Supports the request Send_Break
//         <o.1>D1 bit
//           <i>Enabled = Supports the following requests: Set_Line_Coding, Get_Line_Coding,
//           <i> Set_Control_Line_State, and notification Serial_State
//         <o.0>D0 bit
//           <i>Enabled = Supports the following requests: Set_Comm_Feature, Clear_Comm_Feature and Get_Comm_Feature
//       </h>
#define USBD_CDC0_ACM_ACM_BM_CAPABILITIES 0x06

//       <o>Maximum Communication Device Send Buffer Size
//       <i>Specifies size of buffer used for sending of data to USB Host.
//         <8=>      8 Bytes <16=>    16 Bytes <32=>    32 Bytes <64=>      64 Bytes
//         <128=>  128 Bytes <256=>  256 Bytes <512=>  512 Bytes <1024=>  1024 Bytes
//         <2048=>2048 Bytes <4096=>4096 Bytes <8192=>8192 Bytes <16384=>16384 Bytes
#define USBD_CDC0_ACM_SEND_BUF_SIZE      1024

//       <o>Maximum Communication Device Receive Buffer Size
//       <i>Specifies size of buffer used for receiving of data from USB Host.
//       <i>Minimum size must be twice as large as Maximum Packet Size for Bulk OUT Endpoint.
//       <i>Suggested size is three or more times larger then Maximum Packet Size for Bulk OUT Endpoint.
//         <8=>      8 Bytes <16=>    16 Bytes <32=>    32 Bytes <64=>      64 Bytes
//         <128=>  128 Bytes <256=>  256 Bytes <512=>  512 Bytes <1024=>  1024 Bytes
//         <2048=>2048 Bytes <4096=>4096 Bytes <8192=>8192 Bytes <16384=>16384 Bytes
#define USBD_CDC0_ACM_RECEIVE_BUF_SIZE   2048

//     </h>

//     <h>Network Control Model Settings

//       <s.12>MAC Address String
//       <i>Specifies 48-bit Ethernet MAC address.
#define USBD_CDC0_NCM_MAC_ADDRESS        L"1E306CA2455E"

//       <h>Ethernet Statistics
//       <i>Specifies Ethernet statistic functions supported.
//         <o.0>XMIT_OK
//         <i>Frames transmitted without errors
//         <o.1>RVC_OK
//         <i>Frames received without errors
//         <o.2>XMIT_ERROR
//         <i>Frames not transmitted, or transmitted with errors
//         <o.3>RCV_ERROR
//         <i>Frames received with errors that are not delivered to the USB host.
//         <o.4>RCV_NO_BUFFER
//         <i>Frame missed, no buffers
//         <o.5>DIRECTED_BYTES_XMIT
//         <i>Directed bytes transmitted without errors
//         <o.6>DIRECTED_FRAMES_XMIT
//         <i>Directed frames transmitted without errors
//         <o.7>MULTICAST_BYTES_XMIT
//         <i>Multicast bytes transmitted without errors
//         <o.8>MULTICAST_FRAMES_XMIT
//         <i>Multicast frames transmitted without errors
//         <o.9>BROADCAST_BYTES_XMIT
//         <i>Broadcast bytes transmitted without errors
//         <o.10>BROADCAST_FRAMES_XMIT
//         <i>Broadcast frames transmitted without errors
//         <o.11>DIRECTED_BYTES_RCV
//         <i>Directed bytes received without errors
//         <o.12>DIRECTED_FRAMES_RCV
//         <i>Directed frames received without errors
//         <o.13>MULTICAST_BYTES_RCV
//         <i>Multicast bytes received without errors
//         <o.14>MULTICAST_FRAMES_RCV
//         <i>Multicast frames received without errors
//         <o.15>BROADCAST_BYTES_RCV
//         <i>Broadcast bytes received without errors
//         <o.16>BROADCAST_FRAMES_RCV
//         <i>Broadcast frames received without errors
//         <o.17>RCV_CRC_ERROR
//         <i>Frames received with circular redundancy check (CRC) or frame check sequence (FCS) error
//         <o.18>TRANSMIT_QUEUE_LENGTH
//         <i>Length of transmit queue
//         <o.19>RCV_ERROR_ALIGNMENT
//         <i>Frames received with alignment error
//         <o.20>XMIT_ONE_COLLISION
//         <i>Frames transmitted with one collision
//         <o.21>XMIT_MORE_COLLISIONS
//         <i>Frames transmitted with more than one collision
//         <o.22>XMIT_DEFERRED
//         <i>Frames transmitted after deferral
//         <o.23>XMIT_MAX_COLLISIONS
//         <i>Frames not transmitted due to collisions
//         <o.24>RCV_OVERRUN
//         <i>Frames not received due to overrun
//         <o.25>XMIT_UNDERRUN
//         <i>Frames not transmitted due to underrun
//         <o.26>XMIT_HEARTBEAT_FAILURE
//         <i>Frames transmitted with heartbeat failure
//         <o.27>XMIT_TIMES_CRS_LOST
//         <i>Times carrier sense signal lost during transmission
//         <o.28>XMIT_LATE_COLLISIONS
//         <i>Late collisions detected
//       </h>
#define USBD_CDC0_NCM_BM_ETHERNET_STATISTICS     0x00000003

//       <o>Maximum Segment Size
//       <i>Specifies maximum segment size that Ethernet device is capable of supporting.
//       <i>Typically 1514 bytes.
#define USBD_CDC0_NCM_W_MAX_SEGMENT_SIZE         1514

//       <o.15>Multicast Filtering <0=>Perfect (no hashing) <1=>Imperfect (hashing)
//       <i>Specifies multicast filtering type.
//       <o.0..14>Number of Multicast Filters
//       <i>Specifies number of multicast filters that can be configured by the USB Host.
#define USBD_CDC0_NCM_W_NUMBER_MC_FILTERS        1

//       <o.0..7>Number of Power Filters
//       <i>Specifies number of pattern filters that are available for causing wake-up of the USB Host.
#define USBD_CDC0_NCM_B_NUMBER_POWER_FILTERS     0

//       <h>Network Capabilities
//       <i>Specifies which functions are supported.
//         <o.4>SetCrcMode/GetCrcMode
//         <o.3>SetMaxDatagramSize/GetMaxDatagramSize
//         <o.1>SetNetAddress/GetNetAddress
//         <o.0>SetEthernetPacketFilter
//       </h>
#define USBD_CDC0_NCM_BM_NETWORK_CAPABILITIES    0x1B

//       <h>NTB Parameters
//       <i>Specifies NTB parameters reported by GetNtbParameters function.

//         <h>NTB Formats Supported (bmNtbFormatsSupported)
//         <i>Specifies NTB formats supported.
//           <o.0>16-bit NTB (always supported)
//           <o.1>32-bit NTB
//         </h>
#define USBD_CDC0_NCM_BM_NTB_FORMATS_SUPPORTED   0x0001

//         <h>IN Data Pipe
//
//           <o>Maximum NTB Size (dwNtbInMaxSize)
//           <i>Specifies maximum IN NTB size in bytes.
#define USBD_CDC0_NCM_DW_NTB_IN_MAX_SIZE         4096

//           <o.0..15>NTB Datagram Payload Alignment Divisor (wNdpInDivisor)
//           <i>Specifies divisor used for IN NTB Datagram payload alignment.
#define USBD_CDC0_NCM_W_NDP_IN_DIVISOR           4

//           <o.0..15>NTB Datagram Payload Alignment Remainder (wNdpInPayloadRemainder)
//           <i>Specifies remainder used to align input datagram payload within the NTB.
//           <i>(Payload Offset) % (wNdpInDivisor) = wNdpInPayloadRemainder
#define USBD_CDC0_NCM_W_NDP_IN_PAYLOAD_REMINDER  0

//           <o.0..15>NDP Alignment Modulus in NTB (wNdpInAlignment)
//           <i>Specifies NDP alignment modulus for NTBs on the IN pipe.
//           <i>Shall be power of 2, and shall be at least 4.
#define USBD_CDC0_NCM_W_NDP_IN_ALIGNMENT         4

//         </h>

//         <h>OUT Data Pipe
//
//           <o>Maximum NTB Size (dwNtbOutMaxSize)
//           <i>Specifies maximum OUT NTB size in bytes.
#define USBD_CDC0_NCM_DW_NTB_OUT_MAX_SIZE        4096

//           <o.0..15>NTB Datagram Payload Alignment Divisor (wNdpOutDivisor)
//           <i>Specifies divisor used for OUT NTB Datagram payload alignment.
#define USBD_CDC0_NCM_W_NDP_OUT_DIVISOR          4

//           <o.0..15>NTB Datagram Payload Alignment Remainder (wNdpOutPayloadRemainder)
//           <i>Specifies remainder used to align output datagram payload within the NTB.
//           <i>(Payload Offset) % (wNdpOutDivisor) = wNdpOutPayloadRemainder
#define USBD_CDC0_NCM_W_NDP_OUT_PAYLOAD_REMINDER 0

//           <o.0..15>NDP Alignment Modulus in NTB (wNdpOutAlignment)
//           <i>Specifies NDP alignment modulus for NTBs on the IN pipe.
//           <i>Shall be power of 2, and shall be at least 4.
#define USBD_CDC0_NCM_W_NDP_OUT_ALIGNMENT        4

//         </h>

//       </h>

//       <o.0>Raw Data Access API
//       <i>Enables or disables Raw Data Access API.
#define USBD_CDC0_NCM_RAW_ENABLE         0

//       <o>IN NTB Data Buffering <1=>Single Buffer <2=>Double Buffer
//       <i>Specifies buffering used for sending data to USB Host.
//       <i>Not used when RAW Data Access API is enabled.
#define USBD_CDC0_NCM_NTB_IN_BUF_CNT     1

//       <o>OUT NTB Data Buffering <1=>Single Buffer <2=>Double Buffer
//       <i>Specifies buffering used for receiving data from USB Host.
//       <i>Not used when RAW Data Access API is enabled.
#define USBD_CDC0_NCM_NTB_OUT_BUF_CNT    1

//     </h>

//   </h>

//   <h>OS Resources Settings
//   <i>These settings are used to optimize usage of OS resources.
//     <o>Communication Device Class Interrupt Endpoint Thread Stack Size <64-65536>
#define USBD_CDC0_INT_THREAD_STACK_SIZE  512

//        Communication Device Class Interrupt Endpoint Thread Priority
#define USBD_CDC0_INT_THREAD_PRIORITY    osPriorityAboveNormal

//     <o>Communication Device Class Bulk Endpoints Thread Stack Size <64-65536>
#define USBD_CDC0_BULK_THREAD_STACK_SIZE 512

//        Communication Device Class Bulk Endpoints Thread Priority
#define USBD_CDC0_BULK_THREAD_PRIORITY   osPriorityAboveNormal

//   </h>
// </h>
