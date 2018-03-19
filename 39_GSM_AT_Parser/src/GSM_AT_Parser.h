/*
 ============================================================================
 Name        : GSM_AT_Parser.h
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#ifndef GSM_AT_PARSER_H_
#define GSM_AT_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gsm_config.h"
#include "gsm_ll.h"
#if GSM_RTOS
#include "gsm_sys.h"
#endif

#define __DEBUG(fmt, ...)                   printf(fmt, ##__VA_ARGS__)

typedef enum _GSM_Result_t {
	gsmOK = 0x00, //正确
	gsmERROR, //错误
	gsmBUSY, //忙
	gsmPARERROR, //参数错误
	gsmTIMEOUT, //超时
	gsmSENDFAIL, //发送错误

	gsmSYSERROR, //系统调用错误
	gsmLLERROR, //底层驱动错误
	gsmSIMNOTREADYERROR, //SIM卡没有准备好
	gsmENTERTEXTMODEERROR, //短信模式错误
	gsmSIMMEMORYERROR, //SIM内存出错

	gsmNETWORKNOTREGISTEREDERROR, //网络没有注册
	gsmNETWORKNOTREGISTEREDSEARCHINERROR, //网络正在搜索中
	gsmNETWORKREGISTRATIONDENIEDERROR, //网络注册被拒绝
	gsmNETWORKERROR, //网络错误
} GSM_Result_t;

typedef enum _GSM_CallDir_t {
	GSM_CallDir_MO = 0x00, //电话拨号方
	GSM_CallDir_MT = 0x01 //电话接收方
} GSM_CallDir_t;

typedef enum _GSM_CallType_t {
	GSM_CallType_Voice = 0x00, //语音通话
	GSM_CallType_Data = 0x01, //数据通话
	GSM_CallType_Fax = 0x02 //传真通话
} GSM_CallType_t;

typedef enum _GSM_CallState_t {
	GSM_CallState_Active = 0x00, //通过活动状态
	GSM_CallState_Held = 0x01, //通话被挂起
	GSM_CallState_Dialing = 0x02, //正在拨号
	GSM_CallState_Alerting = 0x03, //正在响铃
	GSM_CallState_Incoming = 0x04, //通话将要到来，等待本地反应
	GSM_CallState_Waiting = 0x05, //通话正在等待状态
	GSM_CallState_Disconnect = 0x06 //通话结束
} GSM_CallState_t;

typedef enum _GSM_SMS_Memory_t {
	GSM_SMS_Memory_SM, GSM_SMS_Memory_BM, GSM_SMS_Memory_SE, GSM_SMS_Memory_ME
} GSM_SMS_Memory_t;

typedef enum _GSM_SMS_MassDelete_t {
	GSM_SMS_MassDelete_Read = 0x00, //删除所有已读短信
	GSM_SMS_MassDelete_Unread, //删除所有未读短信
	GSM_SMS_MassDelete_Sent, //删除所有已发送短信
	GSM_SMS_MassDelete_Unsent, //删除所有未发送短信
	GSM_SMS_MassDelete_Inbox, //删除所有接收到的短信
	GSM_SMS_MassDelete_All //删除所有短信
} GSM_SMS_MassDelete_t;

typedef enum _GSM_SMS_ReadType_t {
	GSM_SMS_ReadType_ALL, //列出所有短信
	GSM_SMS_ReadType_READ, //列出所有已读短信
	GSM_SMS_ReadType_UNREAD, //列出所有未读短信
	GSM_SMS_ReadType_SENT, //列出所有保存的已发送短信
	GSM_SMS_ReadType_UNSENT //李处所有保存的未发送短信
} GSM_SMS_ReadType_t;

/**
 * SIM卡信息
 */
typedef enum _GSM_CPIN_t {
	GSM_CPIN_Unknown = 0x00,
	GSM_CPIN_Ready, /*!< SIM is ready to use */
	GSM_CPIN_SIM_PIN, /*!< SIM is waiting for PIN number */
	GSM_CPIN_SIM_PUK, /*!< SIM is blocked and is waiting for PUK number */
	GSM_CPIN_PH_SIM_PIN,
	GSM_CPIN_PH_SIM_PUK,
	GSM_CPIN_SIM_PIN2,
	GSM_CPIN_SIM_PUK2
} GSM_CPIN_t;

typedef struct _GSM_Date_t {
	uint8_t Day; //日，1~31
	uint8_t Month; //月，1~12
	uint16_t Year; //年
} GSM_Date_t;

typedef struct _GSM_Time_t {
	uint8_t Hours; //时，24小时制
	uint8_t Minutes; //分
	uint8_t Seconds; //秒
} GSM_Time_t;

typedef struct _GSM_DateTime_t {
	GSM_Date_t Date;
	GSM_Time_t Time;
} GSM_DateTime_t;

typedef struct _GSM_SMS_t {
	const char* Number; //号码
	const char* Data; //短信内容
	uint16_t SentSMSMemNum; /*!< Memory number for sent SMS */
} GSM_SMS_t;

typedef struct _GSM_CallInfo_t {
	uint8_t ID; //0~7
	GSM_CallDir_t Dir; //电话方向
	GSM_CallState_t State; //电话状态
	GSM_CallType_t Type; //电话类型
	char Number[16]; //电话号码
	uint8_t IsMultiparty; //是否是multiparty
	uint8_t AddressType; //地址类型
	char Name[20]; //电话簿中的用户名
} GSM_CallInfo_t;

typedef struct _GSM_SmsInfo_t {
	GSM_SMS_Memory_t Memory; //保存短信的内存
	uint16_t Position; //短信保存在内存中的位置
	union {
		struct {
			uint8_t Received :1; //有新短信到来
			uint8_t Used :1; //是都已经占用了内存
			uint8_t UsedByUser :1; //该结构体是否被用户使用
		} F;
		uint8_t Value;
	} Flags;
} GSM_SmsInfo_t;

typedef struct _GSM_SMS_Entry_t {
	uint16_t Position; //在短信内存中的位置
	GSM_SMS_Memory_t Memory; //存放短信的内存类型
	char Data[GSM_SMS_MAX_LENGTH]; //短信内容
	uint8_t DataLen; //短信长度
	char Name[20]; //短信发送者的名字
	char Number[20]; //短信发送者的号码
	GSM_DateTime_t DateTime; //短信发送或者接收的时间
} GSM_SMS_Entry_t;

typedef struct _GSM_PB_Entry_t {
	uint16_t Index; //电话本中的索引号
	char Name[20]; //电话本中记录的名字
	char Number[20]; //电话号码
} GSM_PB_Entry_t;

typedef enum _GSM_CONN_Type_t {
	GSM_CONN_Type_TCP, //创建TCP连接
	GSM_CONN_Type_UDP //创建UDP连接
} GSM_CONN_Type_t;

typedef enum _GSM_CONN_SSL_t {
	GSM_CONN_SSL_Disable = 0x00, //失能TCP over SSL
	GSM_CONN_SSL_Enable = 0x01 //使能TCP over SSL
} GSM_CONN_SSL_t;

typedef struct _GSM_CONN_t {
	uint8_t ID; //连接ID号
	uint8_t* ReceiveData; //指向数据数组的指针
	uint16_t BytesToRead; //一次要读取的字节数
	uint16_t BytesRead; //实际读取的字节数
	uint32_t BytesReadTotal; //总共读取到的字节数
	uint16_t BytesReadRemaining; //当前包中还未读取的字节数
	uint16_t BytesRemaining; //模块的缓冲中还没有读取的字节数
	uint16_t ReadTimeout; //超时时间
	union {
		struct {
			uint8_t Active :1; //连接有效的标志
			uint8_t RxGetReceived :1; //RXGET已经接收到，等待接收数据
			uint8_t CallGetReceived :1; //RXGET已经接收到，提醒用户有新的数据
			uint8_t CallConnClosed :1; //连接被远程服务器关闭
		} F;
		uint8_t Value;
	} Flags;
} GSM_CONN_t;

typedef enum _GSM_HTTP_Method_t {
	GSM_HTTP_Method_GET = 0x00, //GET方法
	GSM_HTTP_Method_POST = 0x01, //POST方法
	GSM_HTTP_Method_HEAD = 0x02 //HEAD方法
} GSM_HTTP_Method_t;

typedef struct _GSM_HTTP_t {
	GSM_HTTP_Method_t Method; //HTTP方法
	const char* TMP; //URL地址
	uint16_t Code; //HTTP返回状态码
	uint8_t* Data; //读写数据指针
	uint32_t DataLength; //数据字节数
	uint32_t BytesReceived; //接收到的字节数
	uint32_t BytesReadTotal; //总共已经读取的字节数
	uint16_t BytesToRead; //一次要读取的字节数
	uint32_t BytesRead; //实际读取的字节数
	uint32_t BytesReadRemaining; //剩余还没读取的字节数
} GSM_HTTP_t;

typedef enum _GSM_HTTP_SSL_t {
	GSM_HTTP_SSL_Disable = 0x00, //失能SSL功能
	GSM_HTTP_SSL_Enable = 0x01 //使能SSL功能
} GSM_HTTP_SSL_t;

typedef struct _GSM_FTP_t {
	uint8_t Mode; //FTP的模式（execute，read）
	uint8_t ErrorCode; //错误代码号
	uint8_t* Data; //读写数据指针
	uint32_t BytesToProcess; //一次要处理的字节数
	uint32_t BytesRead; //实际读取的字节数
	uint32_t BytesReadRemaining; //还没读取的字节数
	uint32_t BytesProcessedTotal; //总共已经读取的字节数
	uint32_t MaxBytesToPut; //可以发送的最大字节数
	union {
		struct {
			uint8_t DataAvailable :1; //如果有数据可读便置1
			uint8_t DownloadActive :1; //如果有数据下载便置1
		} F;
		uint8_t Value;
	} Flags;
} GSM_FTP_t;

typedef enum _GSM_FTP_Mode_t {
	GSM_FTP_Mode_Active = 0x00, //Active模式
	GSM_FTP_Mode_Passive = 0x01 //Passive模式
} GSM_FTP_Mode_t;

typedef enum _GSM_FTP_SSL_t {
	GSM_FTP_SSL_Disable = 0x00, //失能FTP over SSL
	GSM_FTP_SSL_Implicit = 0x01, //隐式使能FTP over SSL
	GSM_FTP_SSL_Explicit = 0x02 //显示使能FTP over SSL
} GSM_FTP_SSL_t;

typedef enum _GSM_FTP_UploadMode_t {
	GSM_FTP_UploadMode_Append = 0x00,
	GSM_FTP_UploadMode_StoreUnique = 0x01,
	GSM_FTP_UploadMode_Store = 0x02
} GSM_FTP_UploadMode_t;

typedef enum _GSM_NetworkStatus_t {
	GSM_NetworkStatus_NotRegistered = 0x00, //还未注册，也不在搜索
	GSM_NetworkStatus_RegisteredHome = 0x01, //在home network中注册
	GSM_NetworkStatus_Searching = 0x02, //搜索网络
	GSM_NetworkStatus_RegistrationDenied = 0x03, //注册被拒绝
	GSM_NetworkStatus_Unknown = 0x04, //未知状态
	GSM_NetworkStatus_RegisteredRoaming = 0x05 //已注册且在漫游中
} GSM_NetworkStatus_t;

typedef struct _GSM_GPS_t {
	uint16_t Error; //错误代码
	float Latitude; //纬度
	float Longitude; //精度
	GSM_Date_t Date; //UTC日期
	GSM_Time_t Time; //UTC时间
} GSM_GPS_t;

typedef struct _GSM_Battery_t {
	uint8_t Charging; //充电中
	uint8_t Percentage; //百分数表示电量
	uint16_t Voltage; //电池电压：毫伏
} GSM_Battery_t;

typedef enum _GSM_OperatorMode_t {
	GSM_OperatorMode_Auto = 0x00, //自动选择运营商网络
	GSM_OperatorMode_Manual = 0x01, //手动选择网络
	GSM_OperatorMode_ManualAuto = 0x04, //先手动选择网络，如果失败就自动选择网络
} GSM_OperatorMode_t;

typedef enum _GSM_OperatorFormat_t {
	GSM_OperatorFormat_LongName = 0x00, //运营商名字长字母数字格式
	GSM_OperatorFormat_ShortName = 0x01, //运营商名字短字母数字格式
	GSM_OperatorFormat_Number = 0x02 //运营商名字按数字格式
} GSM_OperatorFormat_t;

typedef enum _GSM_OperatorStatus_t {
	GSM_OperatorStatus_Unknown = 0x00, //运营商网络未知状态
	GSM_OperatorStatus_Available = 0x01, //运营商网络可用
	GSM_OperatorStatus_Current = 0x02, //运营商网络正在使用
	GSM_OperatorStatus_Forbidden = 0x03 //运营商网络禁止被使用
} GSM_OperatorStatus_t;

typedef struct _GSM_OP_t {
	GSM_OperatorStatus_t Status; //运营商网络状态
	char LongName[20]; //网络长名字
	char ShortName[20]; //网络短名字
	char Number[10]; //网络数字格式名字
} GSM_OP_t;

typedef enum _GSM_Func_t {
	GSM_Func_Min = 0x00, //精简电话功能
	GSM_Func_Full = 0x01, //全部电话功能
	GSM_Func_Disable = 0x04 //飞行模式
} GSM_Func_t;

typedef enum _GSM_Event_t {
	gsmEventIdle = 0x00, //GMS工作栈处于空闲状态
	gsmEventDataReceived, //有新数据收到
	gsmEventDataSent, //数据被发送出去了
	gsmEventDataSentError, //数据发送失败
#if GSM_CALL
	gsmEventCallCLCC, //收到CLCC信号
	gsmEventCallRING, //收到RING信号
#endif /* GSM_CALL */
#if GSM_SMS
	gsmEventSMSCMTI, //收到SMS info信息
#endif /* GSM_SMS */
	gsmEventGPRSAttached, //GPRS业务已经附着
	gsmEventGPRSAttachError, //GPRS业务附着出错
	gsmEventGPRSDetached, //GPRS业务已分离
	gsmEventUVWarning, //收到低压警报
	gsmEventUVPowerDown, //低压断电
} GSM_Event_t;

typedef struct _GSM_EventParams_t {
	const void* CP1;
	const void* CP2;
	uint32_t UI;
} GSM_EventParams_t;

typedef int (*GSM_EventCallback_t)(GSM_Event_t, GSM_EventParams_t*);

typedef struct _GSM_t {
	volatile uint32_t Time; //当前时间，单位毫秒
	volatile GSM_Result_t RetVal; //返回值

	//底层管理
	GSM_LL_t LL; //底层通信

	//有效命令信息
	volatile uint16_t ActiveCmd; //当前可执行的有效命令
	volatile uint16_t ActiveCmdSaved; //保存的有效命令
	const char* volatile ActiveCmdResp; //指向有效命令的返回
	volatile uint32_t ActiveCmdStart; //有效命令开始执行时间
	volatile GSM_Result_t ActiveResult; //函数返回结果
	volatile uint32_t ActiveCmdTimeout; //有效命令超时时间，单位：毫秒

	volatile GSM_NetworkStatus_t NetworkStatus; //网络状态

	GSM_CPIN_t CPIN; //SIM卡状态

	GSM_Func_t Func; //电话功能

	uint8_t IP[4]; //IP地址

	/*!< Plain connections check */
	GSM_CONN_t* Conns[6]; //指向连接的指针数组

#if GSM_SMS
	GSM_SMS_t SMS; //SMS发送对象
	GSM_SmsInfo_t SmsInfos[GSM_MAX_RECEIVED_SMS_INFO]; //接收的SMS对象
#endif /* GSM_SMS */

#if GSM_CALL
	GSM_CallInfo_t CallInfo; //Call对象
#endif /* GSM_CALL */

#if GSM_HTTP
	GSM_HTTP_t HTTP; //HTTP相关结构体
#endif

#if GSM_FTP
	GSM_FTP_t FTP; //FTP相关 结构体
#endif

#if GSM_RTOS
	GSM_RTOS_SYNC_t Sync; //RTOS同步对象
#endif

	union {
		struct {
			uint8_t IsBlocking :1; //函数还行被阻塞
			uint8_t Call_Idle :1; //需要调用IDLE回调
			uint8_t PIN_Ok :1; //PIN码正确
			uint8_t PIN_Error :1; //PIN码不正确
			uint8_t PUK_Ok :1; //PUK码正确
			uint8_t PUK_Error :1; //PUK码不正确

#if GSM_SMS
			uint8_t SMS_SendOk :1; //SMS发送正确
			uint8_t SMS_SendError :1; //SMS发送错误
			uint8_t SMS_Read_Data :1; //正在读取SMS
			uint8_t SMS_CMTI_Received :1; //收到CMTI SMS
#endif /* GSM_SMS */

#if GSM_CALL
			uint8_t CALL_CLCC_Received :1; //收到CLCC call
			uint8_t CALL_RING_Received :1; //收到RING
#endif /* GSM_CALL */

			uint8_t ReadSingleLineDataRespond :1; //读取单行的命令返回值
			uint8_t CLIENT_Read_Data :1; //正在读取来自客户端的返回值

#if GSM_HTTP
			uint8_t HTTP_Read_Data :1; //正在读取HTTP的返回值
#endif /* GSM_HTTP */

#if GSM_FTP
			uint8_t FTP_Read_Data :1; //正在读取FTP的返回值
#endif /* GSM_FTP */

			uint8_t Call_GPRS_Attached :1; //GPRS附着成功
			uint8_t Call_GPRS_Attach_Error :1; //GPRS附着失败
			uint8_t Call_GPRS_Detached :1; //GPRS分离

			uint8_t Call_UV_Warn :1; //收到低电压警报
			uint8_t Call_UV_PD :1; //低压断电

			uint8_t COPS_Read_Operators :1; //需要处理到来的COPS数据

			uint8_t LastOperationStatus :1;
		} F;
		uint32_t Value;
	} Flags;
	GSM_EventCallback_t Callback; //回调函数
	GSM_EventParams_t CallbackParams; //回调函数参数

	union {
		struct {
			uint8_t RespOk :1; //OK信息收到
			uint8_t RespError :1; //收到Error
			uint8_t RespTimeout :1; //超时
			uint8_t RespBracket :1; //收到括号

			uint8_t RespConnectOk :1; //连接成功
			uint8_t RespConnectFail :1; //连接失败
			uint8_t RespConnectAlready :1; //已连接
			uint8_t RespCloseOk :1; //成功关闭
			uint8_t RespSendOk :1; //发送成功
			uint8_t RespSendFail :1; //发送失败

			uint8_t RespCallReady :1; //call已经就绪
			uint8_t RespSMSReady :1; //SMS已经就绪

#if GSM_HTTP
			uint8_t RespHttpAction :1; //收到+HTTPACTION
			uint8_t RespDownload :1; //收到DOWNLOAD
#endif /* GSM_HTTP */

#if GSM_FTP
			uint8_t RespFtpGet :1; //收到FTPGET
			uint8_t RespFtpPut :1; //收到FTPPUT
			uint8_t RespFtpUploadReady :1; //上传数据已经就绪
#endif /* GSM_FTP */
		} F;
		uint32_t Value;
	} Events;
} GSM_t;

#ifdef __cplusplus
}
#endif
#endif /* GSM_AT_PARSER_H_ */
