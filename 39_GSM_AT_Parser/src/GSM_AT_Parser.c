/*
 ============================================================================
 Name        : GSM_AT_Parser.c
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */
#include "GSM_AT_Parser.h"
#include "pt/pt.h"
#include <math.h>

typedef struct {
	uint8_t Length;
	uint8_t Data[128];
} Received_t;

#define RECEIVED_ADD(c)   	do { Received.Data[Received.Length++] = (c); Received.Data[Received.Length] = 0; } while (0)
#define RECEIVED_RESET()    do { Received.Length = 0; Received.Data[0] = 0; } while (0)
#define RECEIVED_LENGTH()   Received.Length

typedef struct {
	volatile const void* CPtr1;
	volatile const void* CPtr2;
	volatile const void* CPtr3;
	volatile void* Ptr1;
	volatile void* Ptr2;
	volatile void* Ptr3;
	volatile uint32_t UI;
} Pointers_t;

#define CHARISNUM(x)          	((x) >= '0' && (x) <= '9')
#define CHARISHEXNUM(x)         (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))
#define CHARTONUM(x)            ((x) - '0')
#define CHARHEXTONUM(x)         (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'z') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'Z') ? ((x) - 'A' + 10) : 0)))
#define FROMMEM(x)              ((const char *)(x))

#define UART_SEND_STR(str)      GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(str), strlen((const char *)(str)))
#define UART_SEND(str, len)     GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(str), (len))
#define UART_SEND_CH(ch)        GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(ch), 1)

#define GSM_OK                  FROMMEM("OK\r\n")
#define GSM_ERROR               FROMMEM("ERROR\r\n")
#define GSM_BUSY                FROMMEM("BUSY\r\n")
#define GSM_NO_CARRIER          FROMMEM("NO CARRIER\r\n")
#define GSM_RING                FROMMEM("RING\r\n")
#define GSM_CRLF                FROMMEM("\r\n")

#define CMD_IDLE                            ((uint16_t)0x0000)
#define CMD_GEN_SMSNOTIFY                   ((uint16_t)0x0001)
#define CMD_GEN_ERROR_NUMERIC               ((uint16_t)0x0002)
#define CMD_GEN_CALL_CLCC                   ((uint16_t)0x0003)
#define CMD_GEN_FACTORY_SETTINGS            ((uint16_t)0x0004)
#define CMD_GEN_AT                          ((uint16_t)0x0005)
#define CMD_GEN_ATE0                        ((uint16_t)0x0006)
#define CMD_GEN_ATE1                        ((uint16_t)0x0007)
#define CMD_GEN_CFUN                        ((uint16_t)0x0008)
#define CMD_GEN_CFUN_SET                    ((uint16_t)0x0009)
#define CMD_GEN_CFUN_GET                    ((uint16_t)0x000A)
#define CMD_IS_ACTIVE_GENERAL(p)            ((p)->ActiveCmd >= 0x0001 && (p)->ActiveCmd < 0x0100)

#define CMD_PIN                             ((uint16_t)0x0101)
#define CMD_PUK                             ((uint16_t)0x0102)
#define CMD_PIN_REMOVE                      ((uint16_t)0x0103)
#define CMD_PIN_ADD                         ((uint16_t)0x0104)
#define CMD_IS_ACTIVE_PIN(p)                ((p)->ActiveCmd >= 0x0100 && (p)->ActiveCmd < 0x0200)

#define CMD_SMS                             ((uint16_t)0x0200)
#define CMD_SMS_SEND                        ((uint16_t)0x0201)
#define CMD_SMS_READ                        ((uint16_t)0x0202)
#define CMD_SMS_DELETE                      ((uint16_t)0x0203)
#define CMD_SMS_MASSDELETE                  ((uint16_t)0x0204)
#define CMD_SMS_LIST                        ((uint16_t)0x0205)
#define CMD_SMS_CMGF                        ((uint16_t)0x0210)
#define CMD_SMS_CMGS                        ((uint16_t)0x0211)
#define CMD_SMS_CMGR                        ((uint16_t)0x0212)
#define CMD_SMS_CMGD                        ((uint16_t)0x0213)
#define CMD_IS_ACTIVE_SMS(p)                ((p)->ActiveCmd >= 0x0200 && (p)->ActiveCmd < 0x0300)

#define CMD_CALL                            ((uint16_t)0x0300)
#define CMD_CALL_VOICE                      ((uint16_t)0x0301)
#define CMD_CALL_DATA                       ((uint16_t)0x0302)
#define CMD_CALL_ANSWER                     ((uint16_t)0x0303)
#define CMD_CALL_HANGUP                     ((uint16_t)0x0304)
#define CMD_CALL_VOICE_SIM_POS              ((uint16_t)0x0305)
#define CMD_CALL_DATA_SIM_POS               ((uint16_t)0x0306)
#define CMD_IS_ACTIVE_CALL(p)               ((p)->ActiveCmd >= 0x0300 && (p)->ActiveCmd < 0x0400)

#define CMD_INFO                            ((uint16_t)0x0400)
#define CMD_INFO_CGMM                       ((uint16_t)0x0401)
#define CMD_INFO_CGMI                       ((uint16_t)0x0402)
#define CMD_INFO_CGMR                       ((uint16_t)0x0403)
#define CMD_INFO_CNUM                       ((uint16_t)0x0404)
#define CMD_INFO_CGSN                       ((uint16_t)0x0405)
#define CMD_INFO_GMR                        ((uint16_t)0x0406)
#define CMD_INFO_CBC                        ((uint16_t)0x0407)
#define CMD_INFO_CSQ                        ((uint16_t)0x0408)
#define CMD_IS_ACTIVE_INFO(p)               ((p)->ActiveCmd >= 0x0400 && (p)->ActiveCmd < 0x0500)

#define CMD_PB                              ((uint16_t)0x0500)
#define CMD_PB_ADD                          ((uint16_t)0x0501)
#define CMD_PB_EDIT                         ((uint16_t)0x0502)
#define CMD_PB_DELETE                       ((uint16_t)0x0503)
#define CMD_PB_GET                          ((uint16_t)0x0504)
#define CMD_PB_LIST                         ((uint16_t)0x0505)
#define CMD_PB_SEARCH                       ((uint16_t)0x0506)
#define CMD_IS_ACTIVE_PB(p)                 ((p)->ActiveCmd >= 0x0500 && (p)->ActiveCmd < 0x0600)

#define CMD_DATETIME                        ((uint16_t)0x0600)
#define CMD_DATETIME_GET                    ((uint16_t)0x0601)
#define CMD_IS_ACTIVE_DATETIME(p)           ((p)->ActiveCmd >= 0x0600 && (p)->ActiveCmd < 0x0700)

#define CMD_GPRS                            ((uint16_t)0x0700)
#define CMD_GPRS_SETAPN                     ((uint16_t)0x0701)
#define CMD_GPRS_ATTACH                     ((uint16_t)0x0702)
#define CMD_GPRS_DETACH                     ((uint16_t)0x0703)
#define CMD_GPRS_HTTPBEGIN                  ((uint16_t)0x0704)
#define CMD_GPRS_HTTPEND                    ((uint16_t)0x0705)
#define CMD_GPRS_HTTPEXECUTE                ((uint16_t)0x0706)
#define CMD_GPRS_HTTPSEND                   ((uint16_t)0x0707)
#define CMD_GPRS_HTTPCONTENT                ((uint16_t)0x0708)
#define CMD_GPRS_FTPBEGIN                   ((uint16_t)0x0709)
#define CMD_GPRS_FTPEND                     ((uint16_t)0x070A)
#define CMD_GPRS_FTPAUTH                    ((uint16_t)0x070B)
#define CMD_GPRS_FTPDOWN                    ((uint16_t)0x070C)
#define CMD_GPRS_FTPDOWNBEGIN               ((uint16_t)0x070D)
#define CMD_GPRS_FTPDOWNEND                 ((uint16_t)0x070E)
#define CMD_GPRS_FTPUP                      ((uint16_t)0x070F)
#define CMD_GPRS_FTPUPBEGIN                 ((uint16_t)0x0710)
#define CMD_GPRS_FTPUPEND                   ((uint16_t)0x0711)

#define CMD_GPRS_CIPSHUT                    ((uint16_t)0x0720)
#define CMD_GPRS_CGATT                      ((uint16_t)0x0721)
#define CMD_GPRS_CGACT                      ((uint16_t)0x0722)
#define CMD_GPRS_SAPBR                      ((uint16_t)0x0723)
#define CMD_GPRS_CIICR                      ((uint16_t)0x0724)
#define CMD_GPRS_CIFSR                      ((uint16_t)0x0725)
#define CMD_GPRS_CSTT                       ((uint16_t)0x0726)
#define CMD_GPRS_CIPMUX                     ((uint16_t)0x0727)
#define CMD_GPRS_CIPSTATUS                  ((uint16_t)0x0728)
#define CMD_GPRS_CIPSTART                   ((uint16_t)0x0729)
#define CMD_GPRS_CIPSEND                    ((uint16_t)0x072A)
#define CMD_GPRS_CIPCLOSE                   ((uint16_t)0x072B)
#define CMD_GPRS_CIPRXGET                   ((uint16_t)0x072C)
#define CMD_GPRS_HTTPINIT                   ((uint16_t)0x072D)
#define CMD_GPRS_HTTPPARA                   ((uint16_t)0x072E)
#define CMD_GPRS_HTTPDATA                   ((uint16_t)0x072F)
#define CMD_GPRS_HTTPACTION                 ((uint16_t)0x0730)
#define CMD_GPRS_HTTPREAD                   ((uint16_t)0x0731)
#define CMD_GPRS_HTTPTERM                   ((uint16_t)0x0732)
#define CMD_GPRS_CREG                       ((uint16_t)0x0733)
#define CMD_GPRS_FTPCID                     ((uint16_t)0x0734)
#define CMD_GPRS_FTPSERV                    ((uint16_t)0x0735)
#define CMD_GPRS_FTPPORT                    ((uint16_t)0x0736)
#define CMD_GPRS_FTPUN                      ((uint16_t)0x0737)
#define CMD_GPRS_FTPPW                      ((uint16_t)0x0738)
#define CMD_GPRS_FTPPUTNAME                 ((uint16_t)0x0739)
#define CMD_GPRS_FTPPUTPATH                 ((uint16_t)0x073A)
#define CMD_GPRS_FTPPUT                     ((uint16_t)0x073B)
#define CMD_GPRS_FTPGETPATH                 ((uint16_t)0x073C)
#define CMD_GPRS_FTPGETNAME                 ((uint16_t)0x073D)
#define CMD_GPRS_FTPGET                     ((uint16_t)0x073E)
#define CMD_GPRS_FTPPUTOPT                  ((uint16_t)0x073F)
#define CMD_GPRS_FTPQUIT                    ((uint16_t)0x0740)
#define CMD_GPRS_FTPMODE                    ((uint16_t)0x0741)
#define CMD_GPRS_HTTPSSL                    ((uint16_t)0x0742)
#define CMD_GPRS_FTPSSL                     ((uint16_t)0x0743)
#define CMD_GPRS_CIPSSL                     ((uint16_t)0x0744)
#define CMD_GPRS_CIPGSMLOC                  ((uint16_t)0x0745)
#define CMD_IS_ACTIVE_GPRS(p)               ((p)->ActiveCmd >= 0x0700 && (p)->ActiveCmd < 0x0800)

#define CMD_OP_SCAN                         ((uint16_t)0x0800)
#define CMD_OP_COPS_SCAN                    ((uint16_t)0x0820)
#define CMD_OP_COPS_READ                    ((uint16_t)0x0821)
#define CMD_OP_COPS_SET                     ((uint16_t)0x0822)
#define CMD_IS_ACTIVE_OP(p)                 ((p)->ActiveCmd >= 0x0800 && (p)->ActiveCmd < 0x0900)

#define __IS_BUSY(p)                        ((p)->ActiveCmd != CMD_IDLE || (p)->Flags.F.Call_Idle != 0)
#define __IS_READY(p)                       (!__IS_BUSY(p))
#define __CHECK_BUSY(p)                     do { if (__IS_BUSY(p)) { __RETURN(p, gsmBUSY); } } while (0)
#define __CHECK_INPUTS(c)                   do { if (!(c)) { __RETURN(GSM, gsmPARERROR); } } while (0)

#if GSM_RTOS == 1
#define __IDLE(GSM)                         do {    \
    if (GSM_SYS_Release((GSM_RTOS_SYNC_t *)&(GSM)->Sync)) { \
    }                                           \
    (GSM)->ActiveCmd = CMD_IDLE;                \
    __RESET_THREADS(GSM);                       \
    if (!(GSM)->Flags.F.IsBlocking) {           \
        (GSM)->Flags.F.Call_Idle = 1;           \
    }                                           \
    memset((void *)&Pointers, 0x00, sizeof(Pointers));  \
} while (0)
#else
#define __IDLE(GSM)                         do {    \
    (GSM)->ActiveCmd = CMD_IDLE;                \
    __RESET_THREADS(GSM);                       \
    if (!(GSM)->Flags.F.IsBlocking) {           \
        (GSM)->Flags.F.Call_Idle = 1;           \
    }                                           \
    memset((void *)&Pointers, 0x00, sizeof(Pointers));  \
} while (0)
#endif

#if GSM_RTOS
#define __ACTIVE_CMD(GSM, cmd)              do {\
    if (GSM_SYS_Request((GSM_RTOS_SYNC_t *)&(GSM)->Sync)) { \
        return gsmTIMEOUT;                      \
    }                                           \
    if ((GSM)->ActiveCmd == CMD_IDLE) {         \
        (GSM)->ActiveCmdStart = (GSM)->Time;    \
    }                                           \
    (GSM)->ActiveCmd = (cmd);                   \
} while (0)
#else
#define __ACTIVE_CMD(GSM, cmd)        do {      \
    if ((GSM)->ActiveCmd == CMD_IDLE) {         \
        (GSM)->ActiveCmdStart = (GSM)->Time;    \
    }                                           \
    (GSM)->ActiveCmd = (cmd);                   \
} while (0)
#endif

#define __CMD_SAVE(GSM)                       (GSM)->ActiveCmdSaved = (GSM)->ActiveCmd
#define __CMD_RESTORE(GSM)                    (GSM)->ActiveCmd = (GSM)->ActiveCmdSaved

#define __RETURN(GSM, val)                      do { (GSM)->RetVal = (val); return (val); } while (0)
#define __RETURN_BLOCKING(GSM, b, mt) do {      \
    GSM_Result_t res;                           \
    (GSM)->ActiveCmdTimeout = mt;               \
    if (!(b)) {                                 \
        (GSM)->Flags.F.IsBlocking = 0;          \
        __RETURN(GSM, gsmOK);                   \
    }                                           \
    (GSM)->Flags.F.IsBlocking = 1;              \
    res = GSM_WaitReady(GSM, mt);               \
    if (res == gsmTIMEOUT) {                    \
        return gsmTIMEOUT;                      \
    }                                           \
    res = (GSM)->ActiveResult;                  \
    (GSM)->ActiveResult = gsmOK;                \
    return res;                                 \
} while (0)

#define __RST_EVENTS_RESP(p)                    do { (p)->Events.Value = 0; } while (0)
#define __CALL_CALLBACK(p, evt)                 (p)->Callback(evt, (GSM_EventParams_t *)&(p)->CallbackParams)

#define GSM_EXECUTE_SIM_READY_CHECK(GSM)  \
if ((GSM)->CPIN != GSM_CPIN_Ready) {                        /* SIM must be ready to call */     \
    __RST_EVENTS_RESP(GSM);                                 /* Reset events */                  \
    UART_SEND_STR(FROMMEM("AT+CPIN?"));                     /* Check again to be sure */        \
    UART_SEND_STR(FROMMEM(GSM_CRLF));                                                           \
    PT_WAIT_UNTIL(pt, (GSM)->Events.F.RespOk ||                                                 \
                        (GSM)->Events.F.RespError);         /* Wait for response */             \
    if ((GSM)->CPIN != GSM_CPIN_Ready) {                                                        \
        GSM->ActiveResult = gsmSIMNOTREADYERROR;            /* SIM is not ready to operate */   \
        __IDLE(GSM);                                        /* Go IDLE mode */                  \
        PT_EXIT(pt);                                        /* Stop execution */                \
    }                                                                                           \
}
#define GSM_EXECUTE_NETWORK_CHECK(GSM)  \
if ((GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredHome && \
    (GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredRoaming) {  /* Check if connected to network */ \
    __CMD_SAVE(GSM);                                                                            \
    __RST_EVENTS_RESP(GSM);                                 /* Reset events */                  \
    UART_SEND_STR(FROMMEM("AT+CREG?"));                     /* Check again to be sure */        \
    UART_SEND_STR(FROMMEM(GSM_CRLF));                                                           \
    StartCommand(GSM, CMD_GPRS_CREG, NULL);                                                     \
    PT_WAIT_UNTIL(pt, (GSM)->Events.F.RespOk ||                                                 \
                        (GSM)->Events.F.RespError);         /* Wait for response */             \
    __CMD_RESTORE(GSM);                                                                         \
    if ((GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredHome &&                             \
        (GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredRoaming) {                          \
        if ((GSM)->NetworkStatus == GSM_NetworkStatus_NotRegistered) {                          \
            (GSM)->ActiveResult = gsmNETWORKNOTREGISTEREDERROR; /* Device is not registered to network */   \
        } else if ((GSM)->NetworkStatus == GSM_NetworkStatus_Searching) {                       \
            (GSM)->ActiveResult = gsmNETWORKNOTREGISTEREDSEARCHINERROR; /* Device is not registered to network */   \
        } else if ((GSM)->NetworkStatus == GSM_NetworkStatus_RegistrationDenied) {              \
            (GSM)->ActiveResult = gsmNETWORKREGISTRATIONDENIEDERROR;    /* Registration denied */   \
        } else {                                                                                \
            (GSM)->ActiveResult = gsmNETWORKERROR;          /* Network ERROR */                 \
        }                                                                                       \
        __IDLE(GSM);                                        /* Go IDLE mode */                  \
        PT_EXIT(pt);                                        /* Stop execution */                \
    }                                                                                           \
}
