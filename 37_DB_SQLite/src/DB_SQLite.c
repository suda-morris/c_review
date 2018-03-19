/*
 ======================================================================
 Name        : DB_SQLite.c
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description : sqlite3数据库的使用
 ======================================================================
 1. 如何改善并发性能：一个数据库文件存放一张表
 2. 动态数据类型：
 NULL：空值
 INTEGER：带符号的整形
 REAL：浮点数，存储为8字节的IEEE浮点数
 TEXT：字符串文本
 BLOB：二进制对象
 3. 每个数据库都保存在一个二进制文件中
 4. 通过数据库级共享锁实现事务处理，并支持原子操作。当多个进程同时向同一数据库写入文件的时候，
 只有一个进程能够抢占到数据库资源
 ======================================================================
 连接SQLite数据库，不存在则创建
 int sqlite3_open(
 const char *filename,	//必须是UTF8编码的数据库文件名
 sqlite3 **ppDb			//SQLite数据库标识符指针
 );
 成功返回0，出错返回错误代码
 ======================================================================
 关闭已经打开的数据库
 int sqlite3_close(sqlite3* db);
 ======================================================================
 执行SQL语句(用来完成增删改的操作)
 int sqlite3_exec(
 sqlite3* db,	//数据库标识符
 const char *sql,	//SQL命令字符串
 int (*callback)(void*,int,char**,char**),//回调函数,当sqlite3_exec函数
 //执行成功时将调用回调函数
 void *,	//第一个回调参数
 char **errmsg	//将错误信息写到此指针指向的地址
 );
 ======================================================================
 执行SQL查询语句
 int SQLITE_STDCALL sqlite3_get_table(
 sqlite3 *db,	//数据库标识符
 const char *zSql,	//SQL查询语句
 char ***pazResult,	//查询结果
 int *pnRow,	//查询结果的行数
 int *pnColumn,	//查询结果的列数
 char **pzErrmsg	//错误信息
 );
 */

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_FILE_NAME	"consume.sqlite"
#define TABLE_MAIN_MENU	"main_menu"
#define TABLE_SUB_MENU	"sub_menu"
#define TABLE_RECORD	"record"
#define ID				"id"
#define MAIN_MENU_ID	"main_id"
#define SUB_MENU_ID		"sub_id"
#define MAIN_MENU_NAME	"main_menu_name"
#define SUB_MENU_NAME	"sub_menu_name"
#define FEE				"fee"
#define TIME			"time"
#define ROWID			"rowid"
#define BUFF_SIZE		(32)

static void execSQL(sqlite3* db, const char* cmd) {
	int ret = 0;
	char* errMsg = NULL;
	char** result;
	int row, col;
	int i;
	if (db) {
		/* 增删改语句 */
		if (strncasecmp(cmd, "select", 6)) {
			ret = sqlite3_exec(db, cmd, NULL, NULL, &errMsg);
			if (ret != SQLITE_OK) {
				printf("exec sql command failed:%d-%s\r\n", ret, errMsg);
			} else {
				printf("exec sql command success\r\n");
			}
		}
		/* 查询语句 */
		else {
			ret = sqlite3_get_table(db, cmd, &result, &row, &col, &errMsg);
			if (ret != SQLITE_OK) {
				printf("exec sql command failed:%d-%s\r\n", ret, errMsg);
			} else {
				printf("exec sql command success\r\n");
				for (i = 0; i < (row + 1) * col; i++) {
					printf("%s%s", result[i],
							((i + 1) % col) ? "\t\t" : "\r\n");
				}
			}
			/* 释放结果所占用的内存空间 */
			sqlite3_free_table(result);
		}
	}
	sqlite3_free(errMsg);
}

int main(int argc, char **argv) {
	sqlite3* db = NULL;
	char sqlCmd[256];
	char buf[BUFF_SIZE];
	int action_select;
	int main_select;
	int sub_select;
	int query_select;
	double fee;
	int ret;
	/* 1. 连接数据库 */
	ret = sqlite3_open_v2(DB_FILE_NAME, &db,
	SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (ret) {
		printf("open sqlite3 database failed:%s\r\n", sqlite3_errmsg(db));
		exit(1);
	} else {
		printf("open a sqlite3 database success\r\n");
	}

	/* 2. 创建数据表 */
	/**
	 * 主菜单数据表		副菜单数据表			消费记录数据表
	 * id	主菜单名	id	主菜单ID	副菜单名	副菜单ID	消费金额	消费时间
	 */
	/* 主菜单数据表 */
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd,
			"create table %s(\
			%s integer primary key not null,\
			%s text)",
			TABLE_MAIN_MENU, ID, MAIN_MENU_NAME);
	execSQL(db, sqlCmd);
	/* 副菜单数据表*/
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd,
			"create table %s(\
			%s integer primary key not null,\
			%s integer not null,\
			%s text);",
			TABLE_SUB_MENU, ID, MAIN_MENU_ID, SUB_MENU_NAME);
	execSQL(db, sqlCmd);
	/* 消费记录数据表 */
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd,
			"create table %s(\
			%s integer not null,\
			%s real default 10.0 check(%s < 20000),\
			%s text);",
			TABLE_RECORD, SUB_MENU_ID, FEE, FEE, TIME);
	execSQL(db, sqlCmd);

	/* 3. 插入数据 */
	/* 插入主菜单数据表 */
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(1,'饮食');", TABLE_MAIN_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(2,'交通');", TABLE_MAIN_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(3,'医疗');", TABLE_MAIN_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(4,'水电');", TABLE_MAIN_MENU);
	execSQL(db, sqlCmd);

	/* 插入副菜单数据表 */
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(1,1,'早饭');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(2,1,'午饭');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(3,1,'晚饭');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(4,1,'点心');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);

	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(5,2,'公交');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(6,2,'打的');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(7,2,'火车');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(8,2,'飞机');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);

	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(9,3,'药费');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(10,3,'挂水费');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(11,3,'手术费');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(12,3,'住院费');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);

	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(13,4,'水');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(14,4,'电');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(15,4,'煤气');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);
	memset(sqlCmd, 0, sizeof(sqlCmd));
	sprintf(sqlCmd, "insert into %s values(16,4,'暖气');", TABLE_SUB_MENU);
	execSQL(db, sqlCmd);

	while (1) {
		printf(
				"可进行的操作选项有：\r\n1. 插入数据\r\n2. 修改数据\r\n3. 删除数据\r\n4. 查询数据\r\n5. 删除数据表\r\n");
		printf("请选择你要进行的操作('q' to quit):");
		if (fgets(buf, BUFF_SIZE, stdin)) {
			if (strncasecmp(buf, "q", 1) == 0) {
				break;
			}
			action_select = atoi(buf);
		} else {
			break;
		}
		/* 插入数据操作 */
		if (action_select == 1) {
			printf("********主菜单********\r\n");
			printf("1. 饮食\r\n2. 交通\r\n3. 医疗\r\n4. 水电\r\n请输入：");
			if (fgets(buf, BUFF_SIZE, stdin)) {
				main_select = atoi(buf);
				switch (main_select) {
				case 1:
					printf("********副菜单********\r\n");
					printf("1. 早饭\r\n2. 午饭\r\n3. 晚饭\r\n4. 点心\r\n请输入：");
					break;
				case 2:
					printf("********副菜单********\r\n");
					printf("1. 公交\r\n2. 打的\r\n3. 火车\r\n4. 飞机\r\n请输入：");
					break;
				case 3:
					printf("********副菜单********\r\n");
					printf("1. 药费\r\n2. 挂水费\r\n3. 手术费\r\n4. 住院费\r\n请输入：");
					break;
				case 4:
					printf("********副菜单********\r\n");
					printf("1. 水\r\n2. 电\r\n3. 煤气\r\n4. 暖气\r\n请输入：");
					break;
				}
				if (fgets(buf, BUFF_SIZE, stdin)) {
					sub_select = atoi(buf);
				} else {
					break;
				}
				printf("请输入消费金额：");
				if (fgets(buf, BUFF_SIZE, stdin)) {
					fee = atof(buf);
				}
			} else {
				break;
			}
			/* 插入消费记录数据表 */
			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "insert into %s values(%d,%f,datetime('now'));",
			TABLE_RECORD, sub_select, fee);
			execSQL(db, sqlCmd);
		}
		/* 4. 修改数据 */
		/* 测试：将第一条消费的金额改为10 */
		else if (action_select == 2) {
			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "update %s set %s=10 where %s=1;", TABLE_RECORD,
			FEE, ROWID);
			execSQL(db, sqlCmd);
		}
		/* 5. 删除数据 */
		/* 测试：将第一条消费记录删除 */
		else if (action_select == 3) {
			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "delete from %s where %s=1;", TABLE_RECORD, ROWID);
			execSQL(db, sqlCmd);

			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "vacuum %s;", TABLE_RECORD);
			execSQL(db, sqlCmd);
		}
		/* 6. 查询数据 */
		else if (action_select == 4) {
			printf(
					"可供查询的信息有：\r\n1. 所有消费历史记录\r\n2. 某一天消费记录\r\n3. 消费总和\r\n 请输入序号： ");
			if (fgets(buf, BUFF_SIZE, stdin)) {
				query_select = atoi(buf);
			} else {
				break;
			}
			switch (query_select) {
			case 1:
				memset(sqlCmd, 0, sizeof(sqlCmd));
				sprintf(sqlCmd,
						"select * from %s left outer join %s on %s.%s=%s.%s order by %s desc;",
						TABLE_SUB_MENU, TABLE_RECORD, TABLE_SUB_MENU, ID,
						TABLE_RECORD, SUB_MENU_ID, TIME);
				execSQL(db, sqlCmd);
				break;
			case 2:
				printf("请输入日期（格式1900-01-01）：");
				scanf("%s", buf);
				getchar();	//将回车符从输入缓存中删掉
				memset(sqlCmd, 0, sizeof(sqlCmd));
				sprintf(sqlCmd, "select * from %s where %s like '%%%s%%';",
				TABLE_RECORD, TIME, buf);
				execSQL(db, sqlCmd);
				break;
			case 3:
				memset(sqlCmd, 0, sizeof(sqlCmd));
				sprintf(sqlCmd, "select sum(%s),%s from %s group by %s;",
				FEE, TIME, TABLE_RECORD, TIME);
				execSQL(db, sqlCmd);
				break;
			default:
				break;
			}
		}
		/* 7. 删除表 */
		else if (action_select == 5) {
			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "drop table %s", TABLE_MAIN_MENU);
			execSQL(db, sqlCmd);
			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "drop table %s", TABLE_SUB_MENU);
			execSQL(db, sqlCmd);
			memset(sqlCmd, 0, sizeof(sqlCmd));
			sprintf(sqlCmd, "drop table %s", TABLE_RECORD);
			execSQL(db, sqlCmd);
		}
	}

	/* 8. 断开数据库连接 */
	ret = sqlite3_close(db);
	if (ret) {
		printf("close sqlite3 database failed:%s\r\n", sqlite3_errmsg(db));
		exit(1);
	} else {
		printf("close sqlite3 database success.\r\n");
	}

	return 0;
}

