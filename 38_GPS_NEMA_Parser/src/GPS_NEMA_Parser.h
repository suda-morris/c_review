/*
 ============================================================================
 Name        : GPS_NEMA_Parser.c
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#ifndef GPS_NEMA_PARSER_H_
#define GPS_NEMA_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Buffer.h"
#include <stddef.h>

/* 最大支持的用户自定义GPS语句 */
#define GPS_CUSTOM_COUNT            (5)
/* 数据接收缓存大小，单位字节 */
#define GPS_BUFFER_SIZE             (512)
/* 最多可见卫星数 */
#define GPS_MAX_SATS_IN_VIEW        (24) 

/*
 * GPS库解析库返回结果，枚举
 */
typedef enum _GPS_Result_t {
	gpsOK, //成功
	gpsERROR, //出错
	gpsNODATA, //无数据
	gpsOLDDATA, //还是旧数据
	gpsNEWDATA //新数据到来
} GPS_Result_t;

/*
 * GPS定位状态
 */
typedef enum _GPS_Fix_t {
	GPS_Fix_Invalid = 0x00, //定位无效
	GPS_Fix_GPS = 0x01, //GPS定位，无差分
	GPS_Fix_DGPS = 0x02 //差分GPS定位，精度更高
} GPS_Fix_t;

/*
 * GPS定位模式
 */
typedef enum GPS_FixMode_t {
	GPS_FixMode_Invalid = 0x01, //无效
	GPS_FixMode_2D = 0x02, //2D定位，获得经纬度，至少需要3颗星
	GPS_FixMode_3D = 0x03 //3D定位，获得经纬度和高度，至少需要4颗星

} GPS_FixMode_t;
/*
 * GPS卫星信息
 */
typedef struct _GPS_Sat_t {
	uint8_t ID; //卫星PRN码编号
	uint8_t Elevation; //卫星仰角
	uint16_t Azimuth; //卫星偏离北方的方位角
	uint8_t SNR; //信噪比，单位dB
} GPS_Sat_t;

/*
 * GPS时间
 */
typedef struct _GPS_Time_t {
	uint8_t Hours; //时，0~23
	uint8_t Minutes; //分
	uint8_t Seconds; //秒
	uint8_t Hundreds; //百分之一秒
	uint16_t Thousands; //千分之一秒
} GPS_Time_t;

/*
 * GPS日期
 */
typedef struct _GPS_Date_t {
	uint8_t Day; //日，1~31
	uint8_t Month; //月，1~12
	uint16_t Year; //年，2xxxx
} GPS_Date_t;

/*
 * GPS速度
 */
typedef enum _GPS_Speed_t {
	/* 公制 */
	GPS_Speed_KilometerPerSecond, //千米每秒
	GPS_Speed_MeterPerSecond, //米每秒
	GPS_Speed_KilometerPerHour, //千米每时
	GPS_Speed_MeterPerMinute, //米每分
	/* 英制 */
	GPS_Speed_MilePerSecond, //英里每秒
	GPS_Speed_MilePerHour, //英里每时
	GPS_Speed_FootPerSecond, //英尺每秒
	GPS_Speed_FootPerMinute, //英尺每分
	/* 下面的速度单位尤其针对跑步者 */
	GPS_Speed_MinutePerKilometer, //分每千米
	GPS_Speed_SecondPerKilometer, //秒每千米
	GPS_Speed_SecondPer100Meters, //秒每百米
	GPS_Speed_MinutePerMile, //分每英里
	GPS_Speed_SecondPerMile, //秒每英里
	GPS_Speed_SecondPer100Yards, //秒每百码，一码=3英尺
	/* 航海制 */
	GPS_Speed_SeaMilePerHour, //海里每时
} GPS_Speed_t;

/*
 * GPS距离与方位
 */
typedef struct _GPS_Distance_t {
	float LatitudeStart; //起点纬度
	float LongitudeStart; //起点经度
	float LatitudeEnd; //终点纬度
	float LongitudeEnd; //终点经度
	float Distance; //起点终点之间的距离
	float Bearing; //起点指向终点的方位角（偏离北）,0~360°
} GPS_Distance_t;

/*
 * 自定义GPS语句中使用的数据类型
 */
typedef enum _GPS_CustomType_t {
	GPS_CustomType_Float, //浮点
	GPS_CustomType_Int, //整型
	GPS_CustomType_String, //字符串
	GPS_CustomType_Char, //字符
	GPS_CustomType_LatLong //经纬度
} GPS_CustomType_t;

/*
 * 自定义GPS语句的解析规则的数据结构
 */
typedef struct _GPS_Custom_t {
	const char* Statement; //语句值，包括开头的‘$’，如“$GPRMC”
	uint8_t TermNumber; //术语编号
	union {
		char S[13]; //将GPS接收到的数据保存为字符串
		char C; //将GPS接收到的数据保存为字符
		float F; //将GPS接收到的数据保存为浮点数
		float L; //将GPS接收到的数据保存为经/纬度（浮点数）
		int I; //将GPS接收到的数据保存为整型数
	} Value; //共同体，不同类型的数据
	GPS_CustomType_t Type; //数据类型
	uint8_t Updated; //更新标志，1表示有了新的数据更新
} GPS_Custom_t;

/*
 * GPS工作结构体
 */
typedef struct _GPS_t {
	/* GPGGA(GPS定位信息)语句中提取 */
	float Latitude; //纬度值
	float Longitude; //经度值
	float Altitude; //海拔值
	GPS_Fix_t Fix; //GPS定位状态
	uint8_t SatsInUse; //使用卫星数量
	GPS_Time_t Time; //UTC时间

	/* GPGSA(当前卫星信息)语句中提取 */
	GPS_FixMode_t FixMode; //GPS定位模式
	uint8_t SatelliteIDs[12]; //12个信道各自正在使用的卫星的PRN码（伪随机噪声码）编号，范围是0~32
	float HDOP; //HDOP水平精度因子
	float PDOP; //PDOP综合位置精度因子
	float VDOP; //VDOP垂直精度因子

	/* GPGSV(可见卫星信息)语句中提取 */
	uint8_t SatsInView; //可见卫星总数
	GPS_Sat_t SatsDesc[GPS_MAX_SATS_IN_VIEW]; //可视范围内的卫星的描述

	/* GPRMC(最简定位信息)语句中提取 */
	GPS_Date_t Date; //UTC日期
	uint8_t Valid; //GPS定位状态有效标志,A:定位；V：导航
	float Speed; //对地航速，单位哩每小时Knots
	float Coarse; //对地航向，以北为参考
	float Variation; //磁偏角

	/* 自定义GPS语句的解析规则 */
	GPS_Custom_t* CustomStatements[GPS_CUSTOM_COUNT]; //指向自定义GPS语句的指针数组
	uint8_t CustomStatementsCount; //自定义GPS语句的数量
} GPS_t;

/**
 * 初始化GPS工作结构体
 * @param  GPS GPS工作结构体指针
 * @return     成功返回gpsOK，失败返回gpsERROR
 */
GPS_Result_t GPS_Init(GPS_t* GPS);

/**
 * 将串口收到的数据复制到内部工作缓存中
 * @param  ch    串口收到的数据
 * @param  count 要写入数据的字节数
 * @return       返回成功写入数据的字节数
 */
uint32_t GPS_DataReceived(uint8_t* ch, size_t count);

/**
 * 执行GPS解析工作
 * @param  GPS GPS工作结构体指针
 * @return     返回GPS解析工作的状态
 */
GPS_Result_t GPS_Update(GPS_t* GPS);

/**
 * 将Knots为单位的速度转换成用户指定的速度单位
 * @param  SpeedInKnots 以Knots为单位的速度值
 * @param  toSpeed      用户指定的速度单位
 * @return              返回后的速度值
 */
float GPS_ConvertSpeed(float SpeedInKnots, GPS_Speed_t toSpeed);

/**
 * 计算两个指定坐标之间的距离以及起点指向终点的方位
 * @param  Distance GPS距离结构体指针
 * @return          成功返回gpsOK
 */
GPS_Result_t GPS_DistanceBetween(GPS_Distance_t* Distance);

/**
 * 添加用户自定义GPS语句中项的解析方式
 * @param  GPS           GPS工作结构体指针
 * @param  Custom        GPS_Custom_t结构体指针
 * @param  GPS_Statement 需要自定义解析格式的GPS语句名字，如$GPRMC
 * @param  TermNumber    项的序号，从1开始
 * @param  Type          需要解析成什么格式
 * @return               成功返回gpsOK，否则返回gpsERROR
 */
GPS_Result_t GPS_Custom_Add(GPS_t* GPS, GPS_Custom_t* Custom,
		const char* GPS_Statement, uint8_t TermNumber, GPS_CustomType_t Type);

/**
 * 删除用户自定义的GPS语句中项的解析方式
 * @param  GPS           GPS工作结构体指针
 * @param  Custom        GPS_Custom_t结构体指针
 * @return        		   成功返回gpsOK，否则返回gpsERROR
 */
GPS_Result_t GPS_Custom_Delete(GPS_t* GPS, GPS_Custom_t* Custom);

#ifdef __cplusplus
}
#endif

#endif /* GPS_NEMA_PARSER_H_ */
