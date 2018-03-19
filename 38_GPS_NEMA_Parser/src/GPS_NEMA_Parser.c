/*
 ============================================================================
 Name        : GPS_NEMA_Parser.c
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include "GPS_NEMA_Parser.h"
#include "Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* GPS解析库内部结构体 */
typedef struct {
	union {
		struct {
			uint8_t Star :1; /* 检测到卫星 */
			uint8_t Term_Num :5; /* 在接收到的字符串中包含的项数 */
			uint8_t Term_Pos :5; /* Term position for adding new character to string object */
			uint8_t Statement :4; /* 识别的语句数 */
			uint8_t GPGSV_Num :3; /* 当前GPGSV语句序号 */
			uint8_t GPGSV_Nums :3; /* 本次GPGSV语句的总数目 */
			uint8_t Received :6; /* Received flags for statements */
		} F;
	} Flags;
	char Term[13]; /* 当前项的字符串 */
	char Statement[7]; /* 当前正在解析的GPS语句的名字 */
	uint8_t CRC; /* 计算得到的CRC校验值 */
} GPS_Int_t;

/* 常用GPS 语句定义 */
#define GPS_UNKNOWN                         (0)
#define GPS_GPGGA                           (1)	//GPS定位信息
#define GPS_GPGSA                           (2)	//当前卫星信息
#define GPS_GPGSV                           (3)	//可见卫星信息
#define GPS_GPRMC                           (4)	//最简定位信息
#define GPS_FLAGS_ALL                       (1 << GPS_GPGGA | 1 << GPS_GPGSA | 1 << GPS_GPGSV | 1 << GPS_GPRMC)

#define CHARISNUM(x)                        ((x) >= '0' && (x) <= '9')
#define CHARISHEXNUM(x)                     (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))
#define CHARTONUM(x)                        ((x) - '0')
#define CHARHEXTONUM(x)                     (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'z') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'Z') ? ((x) - 'A' + 10) : 0)))
#define FROMMEM(x)                          ((const char *)(x))

#define GPS_EARTH_RADIUS                    (6371.0f)//地球半径6371Km
#define GPS_DEGREES2RADIANS(x)              (float)((x) * 0.01745329251994f)  //角度转弧度
#define GPS_RADIANS2DEGREES(x)              (float)((x) * 57.29577951308232f) //弧度转角度

/* 将字符加入CRC校验 */
#define GPS_ADDTOCRC(ch)                    do { Int.CRC ^= (uint8_t)(ch); } while (0)
/* 将字符加入当前项 */
#define GPS_ADDTOTERM(ch)                   do { Int.Term[Int.Flags.F.Term_Pos++] = (ch); Int.Term[Int.Flags.F.Term_Pos] = 0; } while (0);    /* Add new element to term object */
/* 开始处理下一项 */
#define GPS_START_NEXT_TERM()               do { Int.Term[0]= 0; Int.Flags.F.Term_Pos = 0; Int.Flags.F.Term_Num++; } while (0);

#define GPS_CONCAT(x, y)                    (uint16_t)((x) << 8 | (y))

/* GPS解析库内部缓存 */
static uint8_t BufferData[GPS_BUFFER_SIZE];
static BUFFER_t BUFFER;
/* GPS解析库内部处理结构体实例 */
static GPS_Int_t Int;
/* 接收完一轮数据(GPGGA,GPGSV,GPGSA,GPRMC)标志 */
static uint8_t ReceivedFlags = 0x00;

/**
 * 将字符串(只包含十进制数字)转换成整数
 * @param  ptr 带转换的字符串
 * @param  cnt 总共转换了多少个字符
 * @return     返回转换后的数字
 */
static int32_t ParseNumber(const char* ptr, uint8_t* cnt) {
	uint8_t minus = 0, i = 0;
	int32_t sum = 0;

	if (*ptr == '-') { //判断是否是负数
		minus = 1;
		ptr++;
		i++;
	}
	while (CHARISNUM(*ptr)) { /* Parse number */
		sum = 10 * sum + CHARTONUM(*ptr);
		ptr++;
		i++;
	}
	if (cnt != NULL) { /* Save number of characters used for number */
		*cnt = i;
	}
	if (minus) { /* Minus detected */
		return -sum;
	}
	return sum; /* Return number */
}

/**
 * 将字符串(包含十六进制数字)转换成整数
 * @param  ptr 带转换的字符串
 * @param  cnt 总共转换了多少个字符
 * @return     返回转换后的数字
 */
static uint32_t ParseHexNumber(const char* ptr, uint8_t* cnt) {
	uint8_t i = 0;
	uint32_t sum = 0;

	while (CHARISHEXNUM(*ptr)) { /* Parse number */
		sum = 16 * sum + CHARHEXTONUM(*ptr);
		ptr++;
		i++;
	}
	if (cnt != NULL) { /* Save number of characters used for number */
		*cnt = i;
	}
	return sum; /* Return number */
}

/**
 * 将字符串转换成浮点数
 * @param  ptr 带转换的字符串
 * @param  cnt 总共转换了多少个字符
 * @return     返回转换后的数字
 */
static float ParseFloatNumber(const char* ptr, uint8_t* cnt) {
	uint8_t i = 0, j = 0;
	float sum = 0.0f;

	sum = (float) ParseNumber(ptr, &i); /* Parse number */
	j += i;
	ptr += i;
	if (*ptr == '.') { /* Check decimals */
		float dec;
		dec = (float) ParseNumber(ptr + 1, &i) / (float) pow(10, i);
		if (sum >= 0) {
			sum += dec;
		} else {
			sum -= dec;
		}
		j += i + 1;
	}

	if (cnt != NULL) { /* Save number of characters used for number */
		*cnt = j;
	}
	return sum; /* Return number */
}

/* 将字符串转换成经纬度具体数值，单位:度 */
static float ParseLatLong(const char* term) {
	float num;
	uint8_t cnt;
	/* 纬度 */
	if (term[4] == '.') {
		num = (float) (10 * CHARTONUM(term[0]) + CHARTONUM(term[1])); /* Parse degrees */
		num += (float) (10 * CHARTONUM(term[2]) + CHARTONUM(term[3])) / 60.0f; /* Parse minutes */
		num += (float) ParseNumber(&term[5], &cnt)
				/ (60.0f * (float) pow(10, cnt)); /* Parse seconds */
	}
	/* 经度 */
	else {
		num = (float) (100 * CHARTONUM(term[0]) + 10 * CHARTONUM(term[1])
				+ CHARTONUM(term[2])); /* Parse degrees */
		num += (float) (10 * CHARTONUM(term[3]) + CHARTONUM(term[4])) / 60.0f; /* Parse minutes */
		num += (float) ParseNumber(&term[6], &cnt)
				/ (60.0f * (float) pow(10, cnt)); /* Parse seconds */
	}
	return num;
}

/* 解析GPS语句中的每一项 */
static void ParseValue(GPS_t* GPS) {
	uint8_t i;
	if (Int.Flags.F.Term_Num == 0) { //解析第0项，即$开头的协议数据头
		if (strcmp(Int.Term, FROMMEM("$GPGGA")) == 0) {
			Int.Flags.F.Statement = GPS_GPGGA;
		} else if (strcmp(Int.Term, FROMMEM("$GPGSA")) == 0) {
			Int.Flags.F.Statement = GPS_GPGSA;
		} else if (strcmp(Int.Term, FROMMEM("$GPGSV")) == 0) {
			Int.Flags.F.Statement = GPS_GPGSV;
		} else if (strcmp(Int.Term, FROMMEM("$GPRMC")) == 0) {
			Int.Flags.F.Statement = GPS_GPRMC;
		} else {
			Int.Flags.F.Statement = GPS_UNKNOWN;
		}
		strcpy(Int.Statement, Int.Term); /* Copy active string as term statement */
		return;
	}

	/* 检查当前项是否需要根据用户自定义的要求作特殊解析 */
	for (i = 0; i < GPS->CustomStatementsCount; i++) {
		if (Int.Flags.F.Term_Num == GPS->CustomStatements[i]->TermNumber
				&& strcmp(GPS->CustomStatements[i]->Statement, Int.Statement)
						== 0) {
			switch (GPS->CustomStatements[i]->Type) {
			case GPS_CustomType_String: /* Save value as string */
				strcpy(GPS->CustomStatements[i]->Value.S, Int.Term);
				break;
			case GPS_CustomType_Char: /* Save value as character */
				GPS->CustomStatements[i]->Value.C = Int.Term[0];
				break;
			case GPS_CustomType_Int:
				GPS->CustomStatements[i]->Value.I = ParseNumber(Int.Term, NULL);
				break;
			case GPS_CustomType_Float: /* Save value as float */
				GPS->CustomStatements[i]->Value.F = ParseFloatNumber(Int.Term,
				NULL);
				break;
			case GPS_CustomType_LatLong: /* Parse latitude or longitude */
				GPS->CustomStatements[i]->Value.L = ParseLatLong(Int.Term);
				break;
			default:
				break;
			}
			GPS->CustomStatements[i]->Updated = 1; /* Set flag as updated value */
		}
	}

	/* Parse core statements */
	switch (GPS_CONCAT(Int.Flags.F.Statement, Int.Flags.F.Term_Num)) { //Match statement and term
	/* GPGGA语句的解析 */
	case GPS_CONCAT(GPS_GPGGA, 1): //定位点的UTC时间
		GPS->Time.Hours = 10 * CHARTONUM(Int.Term[0]) + CHARTONUM(Int.Term[1]);
		GPS->Time.Minutes =
				10 * CHARTONUM(Int.Term[2]) + CHARTONUM(Int.Term[3]);
		GPS->Time.Seconds =
				10 * CHARTONUM(Int.Term[4]) + CHARTONUM(Int.Term[5]);
		if (Int.Term[6] == '.') {
			uint8_t cnt;
			uint16_t tmp = ParseNumber(&Int.Term[7], &cnt);

			switch (cnt) {
			case 1:
				GPS->Time.Hundreds = 10 * tmp;
				GPS->Time.Thousands = 100 * tmp;
				break;
			case 2:
				GPS->Time.Hundreds = (uint8_t) tmp;
				GPS->Time.Thousands = 10 * tmp;
				break;
			case 3:
				GPS->Time.Hundreds = tmp / 10;
				GPS->Time.Thousands = tmp;
				break;
			}
		}
		break;
	case GPS_CONCAT(GPS_GPGGA, 2): //纬度
		GPS->Latitude = ParseLatLong(Int.Term);
		break;
	case GPS_CONCAT(GPS_GPGGA, 3): //纬度方向，北为正
		if (Int.Term[0] == 'S' || Int.Term[0] == 's') {
			GPS->Latitude = -GPS->Latitude;
		}
		break;
	case GPS_CONCAT(GPS_GPGGA, 4): //经度
		GPS->Longitude = ParseLatLong(Int.Term); /* Parse latitude and save data */
		break;
	case GPS_CONCAT(GPS_GPGGA, 5): //经度方向，东为正
		if (Int.Term[0] == 'W' || Int.Term[0] == 'w') {
			GPS->Longitude = -GPS->Longitude;
		}
		break;
	case GPS_CONCAT(GPS_GPGGA, 6): //GPS定位状态指示
		GPS->Fix = (GPS_Fix_t) CHARTONUM(Int.Term[0]); /* Fix status */
		break;
	case GPS_CONCAT(GPS_GPGGA, 7): //使用卫星数量
		GPS->SatsInUse = ParseNumber(Int.Term, NULL);
		break;
	case GPS_CONCAT(GPS_GPGGA, 9): //海平面高度,单位:米
		GPS->Altitude = ParseFloatNumber(Int.Term, NULL);
		break;

		/* GPGSA语句的解析 */
	case GPS_CONCAT(GPS_GPGSA, 2): //定位类型：二维或者三维
		GPS->FixMode = (GPS_FixMode_t) ParseNumber(Int.Term, NULL);
		break;
	case GPS_CONCAT(GPS_GPGSA, 3):
	case GPS_CONCAT(GPS_GPGSA, 4):
	case GPS_CONCAT(GPS_GPGSA, 5):
	case GPS_CONCAT(GPS_GPGSA, 6):
	case GPS_CONCAT(GPS_GPGSA, 7):
	case GPS_CONCAT(GPS_GPGSA, 8):
	case GPS_CONCAT(GPS_GPGSA, 9):
	case GPS_CONCAT(GPS_GPGSA, 10):
	case GPS_CONCAT(GPS_GPGSA, 11):
	case GPS_CONCAT(GPS_GPGSA, 12):
	case GPS_CONCAT(GPS_GPGSA, 13):
	case GPS_CONCAT(GPS_GPGSA, 14): //各信道正在使用的卫星的RPN码编号
		GPS->SatelliteIDs[Int.Flags.F.Term_Num - 3] = ParseNumber(Int.Term,
		NULL);
		break;
	case GPS_CONCAT(GPS_GPGSA, 15): //PDOP综合位置精度因子
		GPS->PDOP = ParseFloatNumber(Int.Term, NULL);
		break;
	case GPS_CONCAT(GPS_GPGSA, 16): //HDOP水平精度因子
		GPS->HDOP = ParseFloatNumber(Int.Term, NULL);
		break;
	case GPS_CONCAT(GPS_GPGSA, 17): //VDOP垂直精度因子
		GPS->VDOP = ParseFloatNumber(Int.Term, NULL);
		break;

		/* GPRMC语句的解析 */
	case GPS_CONCAT(GPS_GPRMC, 2): //定位状态有效标志，A：定位；V：导航
		GPS->Valid = (Int.Term[0] == 'A');
		break;
	case GPS_CONCAT(GPS_GPRMC, 7): //对地航速，单位Knots
		GPS->Speed = ParseFloatNumber(Int.Term, NULL);
		break;
	case GPS_CONCAT(GPS_GPRMC, 8): //对地航向，以北为参考
		GPS->Coarse = ParseFloatNumber(Int.Term, NULL);
		break;
	case GPS_CONCAT(GPS_GPRMC, 9): //定位点的UTC日期
		GPS->Date.Day = 10 * CHARTONUM(Int.Term[0]) + CHARTONUM(Int.Term[1]);
		GPS->Date.Month = 10 * CHARTONUM(Int.Term[2]) + CHARTONUM(Int.Term[3]);
		GPS->Date.Year = 2000 + 10 * CHARTONUM(Int.Term[4])
				+ CHARTONUM(Int.Term[5]);
		break;
	case GPS_CONCAT(GPS_GPRMC, 10): //磁偏角
		GPS->Variation = ParseFloatNumber(Int.Term, NULL);
		break;

		/* GPGSV语句的解析 */
	case GPS_CONCAT(GPS_GPGSV, 1): //本次GPGSV语句总数目
		Int.Flags.F.GPGSV_Nums = CHARTONUM(Int.Term[0]);
		break;
	case GPS_CONCAT(GPS_GPGSV, 2): //当前GPGSV语句序号
		Int.Flags.F.GPGSV_Num = CHARTONUM(Int.Term[0]);
		break;
	case GPS_CONCAT(GPS_GPGSV, 3): //当前可见卫星总数
		GPS->SatsInView = ParseNumber(Int.Term, NULL);
		break;
	default:
		/* 处理GPGSV语句中的额外语句 */
		if (Int.Flags.F.Statement == GPS_GPGSV && Int.Flags.F.Term_Num >= 4) {
			uint32_t tmp;
			uint8_t mod, term_num;

			tmp = ParseNumber(Int.Term, NULL); /* Parse received number */
			term_num = Int.Flags.F.Term_Num - 4; /* Normalize number */

			mod = term_num % 4; /* Get division by zero */
			term_num = (Int.Flags.F.GPGSV_Num - 1) * 4 + (term_num / 4); /* Calculate array position for data */

			if (term_num < GPS_MAX_SATS_IN_VIEW) {
				switch (mod) {
				case 0: //卫星PRN码编号
					GPS->SatsDesc[term_num].ID = tmp;
					break;
				case 1: //卫星仰角
					GPS->SatsDesc[term_num].Elevation = tmp;
					break;
				case 2: //卫星方位角
					GPS->SatsDesc[term_num].Azimuth = tmp;
					break;
				case 3: //卫星信噪比
					GPS->SatsDesc[term_num].SNR = tmp;
					break;
				default:
					break;
				}
			}
		}
		break;
	}
}

GPS_Result_t GPS_Init(GPS_t*GPS) {
	memset((void *) GPS, 0x00, sizeof(GPS_t)); /* Reset structure for GPS */
	/* Initialize buffer for received data */
	if (BUFFER_Init(&BUFFER, sizeof(BufferData), BufferData) == 0) {
		return gpsOK;
	} else {
		return gpsERROR;
	}
}

uint32_t GPS_DataReceived(uint8_t* ch, size_t count) {
	return BUFFER_Write(&BUFFER, ch, count); /* Write received data to buffer */
}

GPS_Result_t GPS_Update(GPS_t* GPS) {
	uint8_t ch;
	static uint8_t waitingFirst = 1;

	while (BUFFER_Read(&BUFFER, &ch, 1)) { /* Read character by character from device */
		if (ch == '$') { /* Start of string detected */
			memset((void *) &Int, 0x00, sizeof(GPS_Int_t)); /* Reset data structure */
			Int.CRC = 0x00;
			GPS_ADDTOTERM(ch); /* Add character to first term */
		} else if (ch == ',') {
			GPS_ADDTOCRC(ch); /* Compute CRC */
			ParseValue(GPS); /* Check term */
			GPS_START_NEXT_TERM()
			; /* Start next term */
		} else if (ch == '*') {
			Int.Flags.F.Star = 1; /* Star detected */
			ParseValue(GPS); /* Check term */
			GPS_START_NEXT_TERM()
			; /* Start next term */
		} else if (ch == '\r') {
			if ((uint8_t) ParseHexNumber(Int.Term, NULL) == Int.CRC) { /* CRC is OK data valid */
				switch (Int.Flags.F.Statement) {
				case GPS_GPGGA:
				case GPS_GPGSA:
				case GPS_GPRMC:
					ReceivedFlags |= 1 << Int.Flags.F.Statement; //这一轮的该语句处理完成
					break;
				case GPS_GPGSV:
					/* 判断是否处理完所有的GPGSV语句 */
					if (Int.Flags.F.GPGSV_Num == Int.Flags.F.GPGSV_Nums) {
						ReceivedFlags |= 1 << Int.Flags.F.Statement;
					}
					break;
				default:
					break;
				}
			}
		} else if (ch != ' ') { /* Other characters detected */
			if (!Int.Flags.F.Star) { /* If star is not detected yet */
				GPS_ADDTOCRC(ch); /* Compute CRC */
			}
			GPS_ADDTOTERM(ch); /* Add received character to instance */
		}
		if ((ReceivedFlags & GPS_FLAGS_ALL) == GPS_FLAGS_ALL) { /* If all statements are properly received */
			uint8_t ok = 1, i;
			for (i = 0; i < GPS->CustomStatementsCount; i++) { /* Check all custom statements */
				if (!GPS->CustomStatements[i]->Updated) {
					ok = 0;
					break;
				}
			}

			if (ok) {
				ReceivedFlags = 0x00; /* Reset data */
				for (i = 0; i < GPS->CustomStatementsCount; i++) { /* Reset other flags */
					GPS->CustomStatements[i]->Updated = 0;
				}
				waitingFirst = 0; /* Reset flag */
				return gpsNEWDATA; /* We have new data */
			}
		}
	}
	if (waitingFirst) { /* Check if any data anytime received */
		return gpsNODATA; /* No valid data yet */
	}
	return gpsOLDDATA;
}

float GPS_ConvertSpeed(float SpeedInKnots, GPS_Speed_t toSpeed) {
	switch (toSpeed) {
	/* Metric */
	case GPS_Speed_KilometerPerSecond:
		return SpeedInKnots * 0.000514f;
	case GPS_Speed_MeterPerSecond:
		return SpeedInKnots * 0.5144f;
	case GPS_Speed_KilometerPerHour:
		return SpeedInKnots * 1.852f;
	case GPS_Speed_MeterPerMinute:
		return SpeedInKnots * 30.87f;

		/* Imperial */
	case GPS_Speed_MilePerSecond:
		return SpeedInKnots * 0.0003197f;
	case GPS_Speed_MilePerHour:
		return SpeedInKnots * 1.151f;
	case GPS_Speed_FootPerSecond:
		return SpeedInKnots * 1.688f;
	case GPS_Speed_FootPerMinute:
		return SpeedInKnots * 101.3f;

		/* For Runners and Joggers */
	case GPS_Speed_MinutePerKilometer:
		return SpeedInKnots * 32.4f;
	case GPS_Speed_SecondPerKilometer:
		return SpeedInKnots * 1944.0f;
	case GPS_Speed_SecondPer100Meters:
		return SpeedInKnots * 194.4f;
	case GPS_Speed_MinutePerMile:
		return SpeedInKnots * 52.14f;
	case GPS_Speed_SecondPerMile:
		return SpeedInKnots * 3128.0f;
	case GPS_Speed_SecondPer100Yards:
		return SpeedInKnots * 177.7f;

		/* Nautical */
	case GPS_Speed_SeaMilePerHour:
		return SpeedInKnots * 1.0f;
	default:
		return 0;
	}
}

GPS_Result_t GPS_DistanceBetween(GPS_Distance_t* Distance) {
	float f1, f2, l1, l2, df, dfi, a;

	/* Calculate distance between 2 pointes */
	f1 = GPS_DEGREES2RADIANS(Distance->LatitudeStart);
	f2 = GPS_DEGREES2RADIANS(Distance->LatitudeEnd);
	l1 = GPS_DEGREES2RADIANS(Distance->LongitudeStart);
	l2 = GPS_DEGREES2RADIANS(Distance->LongitudeEnd);
	df = GPS_DEGREES2RADIANS(Distance->LatitudeEnd - Distance->LatitudeStart);
	dfi = GPS_DEGREES2RADIANS(
			Distance->LongitudeEnd - Distance->LongitudeStart);

	a = (float) (sin(df * 0.5f) * sin(df * 0.5f)
			+ cos(f1) * cos(f2) * sin(dfi * 0.5f) * sin(dfi * 0.5f));
	Distance->Distance = (float) (GPS_EARTH_RADIUS * 2.0f
			* atan2(sqrt(a), sqrt(1 - a)) * 1000.0f); /* Get distance in meters */

	/* Calculate bearing between two points from point1 to point2 */
	df = (float) (sin(l2 - l1) * cos(f2));
	dfi = (float) (cos(f1) * sin(f2) - sin(f1) * cos(f2) * cos(l2 - l1));
	Distance->Bearing = (float) (GPS_RADIANS2DEGREES(atan2(df, dfi)));

	/* Make bearing always positive from 0 - 360 degrees instead of -180 to 180 */
	if (Distance->Bearing < 0) {
		Distance->Bearing += 360;
	}
	return gpsOK;
}

GPS_Result_t GPS_Custom_Add(GPS_t* GPS, GPS_Custom_t* Custom,
		const char* GPS_Statement, uint8_t TermNumber, GPS_CustomType_t Type) {
	if (GPS->CustomStatementsCount >= GPS_CUSTOM_COUNT) {
		return gpsERROR;
	}

	Custom->Statement = GPS_Statement; /* Save term start name */
	Custom->TermNumber = TermNumber; /* Save term number */
	Custom->Type = Type; /* Save data type */
	Custom->Updated = 0; /* Reset update flag */

	GPS->CustomStatements[GPS->CustomStatementsCount++] = Custom; /* Save pointer to custom object */
	return gpsOK; /* Return OK */
}

GPS_Result_t GPS_Custom_Delete(GPS_t* GPS, GPS_Custom_t* Custom) {
	uint16_t i;
	if (GPS->CustomStatementsCount == 0) {
		return gpsERROR;
	}

	/* Find position */
	for (i = 0; i < GPS->CustomStatementsCount; i++) { /* Find object in array */
		if (GPS->CustomStatements[i] == Custom) {
			break;
		}
	}

	/* Shift others */
	if (i < GPS->CustomStatementsCount) { /* Shift objects up for 1 */
		for (; i < GPS->CustomStatementsCount - 1; i++) {
			GPS->CustomStatements[i] = GPS->CustomStatements[i + 1];
		}
		GPS->CustomStatements[i] = 0; /* Reset last object */
		GPS->CustomStatementsCount--; /* Decrease number of objects */
		return gpsOK; /* Return OK */
	}
	return gpsERROR; /* Return ERROR */
}

