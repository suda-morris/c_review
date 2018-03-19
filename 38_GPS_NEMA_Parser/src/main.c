/*
 * main.c
 *
 *  Created on: 2017年1月18日
 *      Author: morris
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GPS_NEMA_Parser.h"

#define GPGGA_STATEMENT		"$GPGGA,161229.487,3723.2475,N,12158.3416,W,1,07,1.0,9.0,M, , , ,0000*18\r\n"
#define GPGSA_STATEMENT 	"$GPGSA,A,3,07,02,26,27,09,04,15, , , , , ,1.8,1.0,1.5*33\r\n"
#define GPGSV1_STATEMENT 	"$GPGSV,2,1,07,07,79,048,42,02,51,062,43,26,36,256,42,27,27,138,42*71\r\n"
#define GPGSV2_STATEMENT 	"$GPGSV,2,2,07,09,23,313,42,04,19,159,41,15,12,041,42*41\r\n"
#define GPRMC_STATEMENT		"$GPRMC,161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10\r\n"

int main(void) {
	GPS_t GPS;
	GPS_Custom_t Custom;
	GPS_Result_t gpsRes;
	GPS_Distance_t Distance;

	/* 设置要前往的终点经纬度 */
	Distance.LatitudeEnd = 12.2345;
	Distance.LongitudeEnd = 143.432;

	GPS_Init(&GPS);
	/* 添加用户自定义的GPS语句解析格式，将GPRMC中的第三项解析成经纬度 */
	GPS_Custom_Add(&GPS, &Custom, "$GPRMC", 3, GPS_CustomType_LatLong);

	GPS_DataReceived((uint8_t*) GPGGA_STATEMENT, strlen(GPGGA_STATEMENT));
	GPS_DataReceived((uint8_t*) GPGSA_STATEMENT, strlen(GPGSA_STATEMENT));
	GPS_DataReceived((uint8_t*) GPGSV1_STATEMENT, strlen(GPGSV1_STATEMENT));
	GPS_DataReceived((uint8_t*) GPGSV2_STATEMENT, strlen(GPGSV2_STATEMENT));
	GPS_DataReceived((uint8_t*) GPRMC_STATEMENT, strlen(GPRMC_STATEMENT));
	gpsRes = GPS_Update(&GPS);
	if (gpsRes == gpsNEWDATA) {
		if (GPS.Valid) {
			printf("Latitude: %f, Longitude: %f, Altitude: %f\r\n",
					GPS.Latitude, GPS.Longitude, GPS.Altitude);

			/* 打印用户自定义的解析结果*/
			printf("Custom $GPRMC.3 value: %f\r\n", Custom.Value.L);

			Distance.LatitudeStart = GPS.Latitude;
			Distance.LongitudeStart = GPS.Longitude;

			/* 计算距离与方位角 */
			GPS_DistanceBetween(&Distance);

			printf(
					"Distance from current to end positions: %f meters\r\nBearing we should move to reach end coordinate: %f degrees\r\nBearing we are currently moving: %f degrees\r\n",
					Distance.Distance, Distance.Bearing, GPS.Coarse);

		}
	}
	return 0;
}

