#ifndef TMACROS_H_
#define TMACROS_H_

#define _VERSION "1.6"
#define DOC_LIMIT 200

#ifdef _WIN32
#include <process.h>
#define LOCALTIME localtime_s (&timeinfo, &rawtime );
#define POPEN(x, n) _popen(x, n);
#define PCLOSE(x) _pclose(x);
#else
#include <unistd.h>
#define LOCALTIME localtime_r (&rawtime, &timeinfo);
#define POPEN(x, n) popen(x, n);
#define PCLOSE(x) pclose(x);
#endif


#ifdef _WIN32
#define _SLASH "\\"
#else
#define _SLASH "/"
#endif

struct DOC_FLAGS{
	enum Value{
		DOC_SALES_FLAG						= 0x01,
		DOC_OUTGOING_PAYMENT_FLAG			= 0x02,
		DOC_INCOMING_PAYMENT_FLAG			= 0x04,
		DOC_ADDCOSTS_PAYMENT_FLAG			= 0x08
	};
};


#define SAFEDELETE(p) if (p)\
{	delete p; p = NULL;	}

#define PORTS_NUM_BEGIN 20000
#define PORTS_NUM_END 30000

#define MONTH_IN_SEC 2592000
#define DAY_IN_SEC 86400

#define ADMIN_USER 10001

//PRICES
#define PRICE_IN 800
#define PRICE_COST 801
#define PRICE_OUT 802

//Lot Moves
#define PRODUCT_IN 602
#define PRODUCT_OUT 603
#define PRODUCT_RESERVED 604
#define PRODUCT_SUPPLY 605
#define PRODUCT_ORDER_CUSTOMER 606

//Documents
#define DOC_CONTRACT_OUT 1531
#define DOC_CONTRACT_IN 1530
#define DOC_ORDER_OUT 1525
#define DOC_ORDER_IN 1524
#define DOC_SUPPLIER_RETURNS 1523
#define DOC_CUSTOMER_RETURNS 1522
#define DOC_SALES 1521					// rashodnaya nakladnaya
#define DOC_GOODS_RECEIPT 1520			// prihodnaya nakladnaya
#define DOC_QUOTATION 1517
#define	DOC_WAY_BILL 1516				// s4et-factura
#define DOC_INVOICE 1515				// invoice-s4et klientu
#define DOC_TAXES_PAYMENT 1510
#define DOC_ADJUSTMENT_PAYMENT 1509
#define DOC_TRANSFER_PAYMENT 1508
#define DOC_OUTGOING_PAYMENT 1507		// ishodyawiy platej
#define DOC_INCOMING_PAYMENT 1506		// vhodiawiy platej
#define DOC_ADDCOSTS_PAYMENT 1505
#define DOC_GOODS_ENTRY 1502			// vvod tovara na sklad
#define DOC_GOODS_WRITEOFF 1501			// spisanie tovara
#define DOC_MOVE 1503			//
#define DOC_BUNDLE 1504

// Permissions
#define PERMISSION_DOC_CONTRACT_OUT 3531
#define PERMISSION_DOC_CONTRACT_IN 3530
#define PERMISSION_DOC_ORDER_OUT 3525
#define PERMISSION_DOC_ORDER_IN 3524
#define PERMISSION_DOC_SUPPLIER_RETURNS 3523
#define PERMISSION_DOC_CUSTOMER_RETURNS 3522
#define PERMISSION_DOC_SALES 3521					// rashodnaya nakladnaya
#define PERMISSION_DOC_GOODS_RECEIPT 3520			// prihodnaya nakladnaya
#define PERMISSION_DOC_QUOTATION 3517
#define	PERMISSION_DOC_WAY_BILL 3516				// s4et-factura
#define PERMISSION_DOC_INVOICE 3515				// invoice-s4et klientu
#define PERMISSION_DOC_TAXES_PAYMENT 3510
#define PERMISSION_DOC_ADJUSTMENT_PAYMENT 3509
#define PERMISSION_DOC_TRANSFER_PAYMENT 3508
#define PERMISSION_DOC_OUTGOING_PAYMENT 3507		// ishodyawiy platej
#define PERMISSION_DOC_INCOMING_PAYMENT 3506		// vhodiawiy platej
#define PERMISSION_DOC_ADDCOSTS_PAYMENT 3505
#define PERMISSION_DOC_GOODS_ENTRY 3502			// vvod tovara na sklad
#define PERMISSION_DOC_GOODS_WRITEOFF 3501			// spisanie tovara
#define PERMISSION_DOC_MOVE 3503					//
#define PERMISSION_DOC_BUNDLE 3504

//Documents statuses
#define DOC_NEW 700						// noviy doc
#define DOC_DELETED 701					// udalennyu
#define DOC_POSTED 702					// provedenny
#define DOC_SAVED 703					// sohranenniyN
#define DOC_POST_PENDING 704
#define DOC_UNPOST_PENDING 705
#define DOC_POST_ERROR 706
#define DOC_UNPOST_ERROR 707

//DocItems Statuses
#define DOC_ITEM_SAVED 1600
#define DOC_ITEM_RESERVED 1601

//Price categories
#define TO_LOT_COST_PRICE 2054			// na sebestoimost' partii
#define TO_INCOMING_LOT_PRICE 2053		// na cenu prihoda partii
#define TO_AVERAGE_COST_PRICE 2052		// na srednuju sebestoimost'
#define TO_AVERAGE_INCOMING_PRICE 2051	// na srednuju cenu prihoda
#define TO_CATEGORY 2050				// na kategoriju

// Measurement types
#define CURRENCY_TYPE 100				// valuty
#define VOLUME_TYPE 101					// obyom
#define WEIGHT_TYPE 102					// ves
#define LENGTH_TYPE 103					// dlina
#define OTHER_TYPE 104					// drugie

// Products types

#define PRODUCT 250			// product
#define BUNDLE 251						// komplect
#define SERVICE 252						// servise
#define SOFTWARE 253					// soft

// Partner type

#define COMPANY 300
#define PERSON 301

// Partner role

#define OUR_COMPANY 310
#define BANK 311
#define MANUFACTURER 312
#define CLIENT 320
#define SUPPLIER 321
#define CLIENT_AND_SUPPLIER 322
#define EMPLOYEE 323

// Partner Balance status

#define PARTNER_BALANCE_IN 1275
#define PARTNER_BALANCE_OUT 1276
#define PARTNER_BALANCE_DEBET 1280
#define PARTNER_BALANCE_CREDIT 1281
// Payment status

#define PAYMENT_STATUS_IN 650
#define PAYMENT_STATUS_OUT 651

// Service status

#define SERVICE_STATUS_BEGIN 1650
#define SERVICE_STATUS_END 1651
#define SERVICE_STATUS_START 1652
#define SERVICE_STATUS_PAUSE 1653
#define SERVICE_STATUS_STOP 1654
#define SERVICE_STATUS_CANCEL 1655
#define SERVICE_STATUS_NEW 1656

#define OUR_COMPANIES_CATEGORY 3010
#define BANKS_CATEGORY 3011
#define MANUFACTURERS_CATEGORY 3012
#define VENDORS_AND_SUPPLIERS_CATEGORY 3013

#define PRODUCTS_CATEGORY 3000
#define BUNDLES_CATEGORY 3001
#define SERVICES_CATEGORY 3002
#define SOFTWARE_CATEGORY 3003

#define CATEGORY_EMPLOYEES 3014

#define RECEIVED_ON_SUM 2100
#define SUM_RECEIVED 2103
#define SENT_ON_SUM 2101
#define SUM_SENT 2104

#define CRON_TASK_NEW 2110
#define CRON_TASK_INPROGRESS 2111
#define CRON_TASK_DONE 2112
#define CRON_TASK_ERROR 2113

#define TOTAL_SUM 2150
#define TOTAL_SUM_MINUS_TAXES 2151
#define TOTAL_PROFIT 2152
#define TOTAL_INCOME 2153
#define TOTAL_EXPENSE 2154

#define RELATION_TYPE_GENERAL 1900
#define RELATION_TYPE_MAKEDOC 1901
#define RELATION_TYPE_PAYDOC 1902
//

#define PERMISSIONS_DOCUMENTS 2200
#define PERMISSIONS_DIRECTORIES 2201
#define PERMISSIONS_WAREHOUSES 2202
#define PERMISSIONS_PARTNERS 2203

#define PERMISSION_CHANGE_RIGHTS 3563

//
#define DIR_FINCANCES 56
#define DIR_REPORTS 57
#define DIR_INTERNET 59

#define PERMISSION_DIR_FINCANCES 3590
#define PERMISSION_DIR_REPORTS 3600
#define PERMISSION_DIR_WAREHOUSES 3558
#define PERMISSION_DIR_INTERNET 3559
#define PERMISSION_DIR_PRODUCTS 3555
#define PERMISSION_DIR_PARTNERS 3566
#define PERMISSION_DIR_USERS 3567
#define PERMISSION_PRODUCTS_TRANSACTIONS 3565

// BARCODES STATUSES

#define BARCODE_NEW 2270
#define BARCODE_OLD 2271

#define SYSTEM_SETTINGS 2500

// MESSAGES

#define MESSAGE_SYSTEM 2550
#define MESSAGE_NOTIFICATION 2551
#define MESSAGE_USER_IN 2552
#define MESSAGE_USER_OUT 2553

#define MESSAGE_STATUS_NEW 2560
#define MESSAGE_STATUS_READ 2561
#define MESSAGE_STATUS_DELETED 2562

//
#define PHYSICAL_ADDRESS 391
#define LEGAL_ADDRESS 390

#define TAX_VAT 1000

#define EAN14 2301


#endif
