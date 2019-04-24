#ifndef PandoraTest_H
#define PandoraTest_H

#include "http/TSocketManager.h"
#include "TLogger.h"
#include "TConfig.h"

#include <boost/thread/mutex.hpp>
#include "TSession.h"

//#include "TDatabaseReg.h"
#include "TResponseInfo.h"
#include "TRequestInfo.h"
#include "TCollection.h"


#include <vector>
#include "http/request.hpp"

using namespace std;
extern TLogger * logger;
extern TConfig * config;
class TResposnseInfo;
class TRequestInfo;
class TQueue;

class PandoraTest
{
public:

	string pandora_host;
	string pandora_port;

	vector<TSession *> sessions;
	TSocketManager * manager;
	TQueue * queue;
//	TDatabase * db;
	std::vector<boost::shared_ptr<boost::thread> > threads;
	boost::thread::id main_thread_id;

	boost::thread sessions_run;

	boost::mutex mutex;
	boost::mutex pmutex;
	boost::mutex cmutex;

	int max_client_threads;
	int request_processing_threads;

	bool stoping_server;
	bool stop_server;
	bool stop_cron;
	bool cron_new_req;
	bool is_client_window_open;
	bool first_run;

	PandoraTest(string port, string db_name, string conf_path, bool auto_conf);
	~PandoraTest();

	/************************************************************************/
	/* CHRON                                                                     */
	/************************************************************************/
	void StartCron(int sleep_time);
	void StopCron();
	void CronPause();
	void CronResume();

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	request * PrepareRequest(string command, string data, string session, string parameters, string response_type);
	string PrepareHTTPRequest(bool post_req, string post_req_type, string http_url, string host, string request_data);

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	void CheckClientWindow(int sleep_time, bool);
	void CloseServer();

	//Sessions
	TResponseInfo * GetSessionsRequest(request * req);

	TResponseInfo * ValidateParameters(request * req);
	void Init(string port, string db_name, string conf_path, bool auto_conf);
	void RequestProcessor();

	TRequestInfo * ParseRequestInfo(string &req);
	TRequestInfo * ParseRequestInfoJson(string &req);

	map <wstring, wstring> ParseParameters(string params);
	map <wstring, wstring> ParseParametersJson(string params);

	vector<wstring> ParseVectorString(string params);
	vector<wstring> ParseVectorStringJson(string params);

	void UpdateSessionData(TResponseInfo * resp, request * req);
	TResponseInfo * ExecCommand(request * req);
	string PrepareHTTPResponce(string str);
	string GenCookie(int id);
	void ProcessError(TResponseInfo * resp, string error_code, string error_message, string terror, bool send_error = true);

	//Requests
	TResponseInfo * LoginRequest(request * req);
	TResponseInfo * LogoutRequest(request * req);
	TResponseInfo * CheckSessionRequest(request * req);
	TResponseInfo * GetDatabaseListRequest(request * req);

	int GetUserId(string cookie);

	//Language Requests
	TResponseInfo * GetLanguageRequest(request * req);
	TResponseInfo * GetLanguagesRequest(request * req);
	TResponseInfo * RemoveLanguageRequest(request * req);
	TResponseInfo * AddLanguageRequest(request * req);
	TResponseInfo * SaveLanguageRequest(request * req);
	TResponseInfo * ActivateLanguagesRequest(request * req);
    TResponseInfo * SaveDefaultLanguageRequest(request * req);

	//Statuses Requests

	TResponseInfo * GetStatusRequest(request * req);
	TResponseInfo * GetStatusesRequest(request * req);
	TResponseInfo * RemoveStatusRequest(request * req);
	TResponseInfo * AddStatusRequest(request * req);
	TResponseInfo * SaveStatusRequest(request * req);

	//Measurements Request

	TResponseInfo * SaveMeasurementRequest(request * req);
	TResponseInfo * AddMeasurementRequest(request * req);
	TResponseInfo * RemoveMeasurementRequest(request * req);
	TResponseInfo * GetMeasurementRequest(request * req);
	TResponseInfo * GetMeasurementsRequest(request * req);
    TResponseInfo * SaveDefaultCurrencyRequest(request * req);

	//Rates Request

	TResponseInfo * SaveRateRequest(request * req);
	TResponseInfo * AddRateRequest(request * req);
	TResponseInfo * RemoveRateRequest(request * req);
	TResponseInfo * GetRateByIdRequest(request * req);
	TResponseInfo * GetRateRequest(request * req);
	TResponseInfo * GetRateViewRequest(request * req);
	TResponseInfo * GetRatesRequest(request * req);
	TResponseInfo * GetRatesViewRequest(request * req);

	//Categories Request

	TResponseInfo * GetCategoriesRequest(request * req);
	TResponseInfo * SaveCategoryRequest(request * req);
	TResponseInfo * AddCategoryRequest(request * req);
	TResponseInfo * RemoveCategoryRequest(request * req);
	TResponseInfo * GetCategoryRequest(request * req);


	//Addresses Request

	TResponseInfo * SaveAddressRequest(request * req);
	TResponseInfo * AddAddressRequest(request * req);
	TResponseInfo * RemoveAddressRequest(request * req);
	TResponseInfo * GetAddressRequest(request * req);

	//Partners Request

	TResponseInfo * SavePartnerRequest(request * req);
	TResponseInfo * AddPartnerRequest(request * req);
	TResponseInfo * RemovePartnerRequest(request * req);
	TResponseInfo * GetPartnerRequest(request * req);
	TResponseInfo * GetPartnersRequest(request * req);

	//AddPerson Request
	TResponseInfo * AddPersonRequest(request * req);
	TResponseInfo * SavePersonRequest(request * req);
	TResponseInfo * RemovePersonRequest(request * req);
	TResponseInfo * GetPersonRequest(request * req);

	//Companies Request

	TResponseInfo * AddCompanyRequest(request * req);
	TResponseInfo * SaveCompanyRequest(request * req);
	TResponseInfo * RemoveCompanyRequest(request * req);
	TResponseInfo * GetCompanyRequest(request * req);

	//Employees Request

	TResponseInfo * AddEmployeeRequest(request * req);
	TResponseInfo * SaveEmployeeRequest(request * req);
	TResponseInfo * RemoveEmployeeRequest(request * req);
	TResponseInfo * GetEmployeeRequest(request * req);
	TResponseInfo * GetEmployeesRequest(request * req);

	TResponseInfo * GetEmployeesViewRequest(request * req);

	/************************************************************************/
	/* MANAGERS                                                             */
	/************************************************************************/
	TResponseInfo * GetManagersViewRequest(request * req);

	/************************************************************************/
	/* CONTACT PERSONS                                                      */
	/************************************************************************/
	TResponseInfo * AddContactPersonRequest(request * req);
	TResponseInfo * SaveContactPersonRequest(request * req);
	TResponseInfo * RemoveContactPersonRequest(request * req);
	TResponseInfo * GetContactPersonRequest(request * req);
	TResponseInfo * GetContactPersonsViewRequest(request * req);

	//PartnerView Request

	TResponseInfo * GetPartnerViewRequest(request * req);
	TResponseInfo * GetPartnersViewRequest(request * req);
	TResponseInfo * GetPartnersSearchViewRequest(request * req);

	TResponseInfo * SavePartnerNameRequest(request * req);

	/************************************************************************/
	/* TPARTNER BALANCES                                                    */
	/************************************************************************/
	TResponseInfo * GetPartnerBalanceViewRequest(request * req);
	TResponseInfo * GetPartnerBalancesViewRequest(request * req);

	//SettingsRequest

	TResponseInfo * GetSettingsRequest(request * req);
	TResponseInfo * SetSettingsRequest(request * req);

	//Cat_Objects
	TResponseInfo * SaveCategoriesByObjectRequest(request * req);
	TResponseInfo * GetCategoriesByObjectRequest(request * req);

	/************************************************************************/
	/* GET OBJECT ID                                                        */
	/************************************************************************/
	TResponseInfo * GetObjectIdRequest(request * req);

	//ProductsRequest
	TResponseInfo * AddProductRequest(request * req);
	TResponseInfo * GetProductRequest(request * req);
	TResponseInfo * RemoveProductRequest(request * req);

	TResponseInfo * AddTangibleProductRequest(request * req);
	TResponseInfo * SaveTangibleProductRequest(request * req);
	TResponseInfo * RemoveTangibleProductRequest(request * req);
	TResponseInfo * GetTangibleProductRequest(request * req);
	TResponseInfo * GetTangibleProductsRequest(request * req);

	TResponseInfo * GetTangibleProductViewRequest(request * req);

	TResponseInfo * GetProductsSearchViewRequest(request * req);
	TResponseInfo * GetProductsViewRequest(request * req);
	TResponseInfo * GetProductViewRequest(request * req);
	TResponseInfo * GetProductsViewSummaryRequest(request * req);
	TResponseInfo * GetProductViewSummaryRequest(request * req);

	/************************************************************************/
	/*   MINIMAL STOCK LEVEL                                                */
	/************************************************************************/
	TResponseInfo * SaveProductMinStockLevelsRequest(request * req);
	TResponseInfo * GetProductMinStockLevelsViewRequest(request * req);

	//ProductSubs Request

	TResponseInfo * AddProductSubRequest(request * req);
	TResponseInfo * SaveProductSubRequest(request * req);
	TResponseInfo * RemoveProductSubRequest(request * req);
	TResponseInfo * GetProductSubRequest(request * req);
	TResponseInfo * GetProductSubViewRequest(request * req);

	TResponseInfo * GetProductSubsRequest(request * req);
	TResponseInfo * GetProductSubsViewRequest(request * req);


	//Images Request

	TResponseInfo * AddImageRequest(request * req);
	TResponseInfo * SaveImageRequest(request * req);
	TResponseInfo * RemoveImageRequest(request * req);
	TResponseInfo * GetImageRequest(request * req);
	TResponseInfo * GetImagesRequest(request * req);

	//Product Kit

	TResponseInfo * AddProductKitRequest(request * req);
	TResponseInfo * SaveProductKitRequest(request * req);
	TResponseInfo * RemoveProductKitRequest(request * req);
	TResponseInfo * GetProductKitRequest(request * req);
	TResponseInfo * GetProductKitsRequest(request * req);

	TResponseInfo * GetProductKitsViewRequest(request * req);
	TResponseInfo * GetProductKitViewRequest(request * req);

	//Bundle

	TResponseInfo * AddBundleRequest(request * req);
	TResponseInfo * SaveBundleRequest(request * req);
	TResponseInfo * RemoveBundleRequest(request * req);
	TResponseInfo * GetBundleRequest(request * req);
	TResponseInfo * GetBundleViewRequest(request * req);

	//Service

	TResponseInfo * AddServiceRequest(request * req);
	TResponseInfo * SaveServiceRequest(request * req);
	TResponseInfo * RemoveServiceRequest(request * req);
	TResponseInfo * GetServiceRequest(request * req);
	TResponseInfo * GetServiceViewRequest(request * req);

	//SoftWare

	TResponseInfo * AddSoftwareRequest(request * req);
	TResponseInfo * SaveSoftwareRequest(request * req);
	TResponseInfo * RemoveSoftwareRequest(request * req);
	TResponseInfo * GetSoftwareRequest(request * req);
	TResponseInfo * GetSoftwareViewRequest(request * req);

	/************************************************************************/
	/* WAREHOUSES                                                           */
	/************************************************************************/

	TResponseInfo * SaveWarehouseRequest(request * req);
	TResponseInfo * AddWarehouseRequest(request * req);
	TResponseInfo * RemoveWarehouseRequest(request * req);
	TResponseInfo * GetWarehouseRequest(request * req);
	TResponseInfo * GetWarehousesRequest(request * req);

	/************************************************************************/
	/* USERS                                                                */
	/************************************************************************/

	TResponseInfo * SaveUserRequest(request * req);
	TResponseInfo * AddUserRequest(request * req);
	TResponseInfo * RemoveUserRequest(request * req);
	TResponseInfo * GetUserRequest(request * req);
	TResponseInfo * GetUserViewRequest(request * req);
	TResponseInfo * GetUsersRequest(request * req);
	TResponseInfo * GetUsersViewRequest(request * req);
	TResponseInfo * SaveUserPasswordRequest(request * req);
	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	TResponseInfo * UpdateUserStatusRequest(request * req);

	/************************************************************************/
	/* DOCUMENTS                                                            */
	/************************************************************************/
	TResponseInfo * GetDocumentTypeIdRequest(request * req);
	TResponseInfo * GetDocumentsRequest(request * req);
	TResponseInfo * GetDocumentsViewRequest(request * req);
	TResponseInfo * GetDocumentViewRequest(request * req);

	/************************************************************************/
	/* Documents Put In                                                     */
	/************************************************************************/

	TResponseInfo * SavePutInDocumentRequest(request * req);
	TResponseInfo * AddPutInDocumentRequest(request * req);
	TResponseInfo * RemovePutInDocumentRequest(request * req);
	TResponseInfo * GetPutInDocumentRequest(request * req);
	TResponseInfo * GetPutInDocumentViewRequest(request * req);
	TResponseInfo * GetPutInDocumentsRequest(request * req);
	TResponseInfo * GetPutInDocumentsViewRequest(request * req);

	TResponseInfo * PostPutInDocumentRequest(request * req);
	TResponseInfo * UnpostPutInDocumentRequest(request * req);
	/************************************************************************/
	/* DOC        ITEMS                                                     */
	/************************************************************************/

	TResponseInfo * AddDocItemRequest(request * req);
	TResponseInfo * AddPutInDocItemRequest(request * req);
	TResponseInfo * SavePutInDocItemRequest(request * req);
	TResponseInfo * SavePutInDocItemsRequest(request * req);
	TResponseInfo * RemovePutInDocItemRequest(request * req);
	TResponseInfo * GetPutInDocItemRequest(request * req);
	TResponseInfo * GetPutInDocItemsViewRequest(request * req);

	/************************************************************************/
	/* TAXES                                                               */
	/************************************************************************/
	TResponseInfo * SaveTaxRequest(request * req);
	TResponseInfo * AddTaxRequest(request * req);
	TResponseInfo * RemoveTaxRequest(request * req);
	TResponseInfo * GetTaxRequest(request * req);
	TResponseInfo * GetTaxesRequest(request * req);
	TResponseInfo * GetTaxesViewRequest(request * req);

	TResponseInfo * AddCustomTaxesRequest(request * req);

	/************************************************************************/
	/* DISCOUNTS                                                            */
	/************************************************************************/
	TResponseInfo * GetDiscountsViewRequest(request * req);
	TResponseInfo * AddCustomDiscountsRequest(request * req);

	/************************************************************************/
	/* HISTORY                                                              */
	/************************************************************************/
	TResponseInfo * GetLogsViewRequest(request * req);

	/************************************************************************/
	/* WRITEOFF DOCUMENTS                                                   */
	/************************************************************************/
	TResponseInfo * AddWriteOffDocumentRequest(request * req);
	TResponseInfo * SaveWriteOffDocumentRequest(request * req);
	TResponseInfo * RemoveWriteOffDocumentRequest(request * req);
	TResponseInfo * GetWriteOffDocumentRequest(request * req);
	TResponseInfo * GetWriteOffDocumentViewRequest(request * req);
	TResponseInfo * GetWriteOffDocumentsRequest(request * req);
	TResponseInfo * GetWriteOffDocumentsViewRequest(request * req);
	TResponseInfo * PostWriteOffDocumentRequest(request * req);
	TResponseInfo * UnpostWriteOffDocumentRequest(request * req);


	/************************************************************************/
	/* LOTS                                                                 */
	/************************************************************************/
	TResponseInfo * GetLotViewRequest(request * req);
	TResponseInfo * GetLotsViewRequest(request * req);
	TResponseInfo * GetDocItemLotsViewRequest(request * req);
	TResponseInfo * GetDocItemLotViewRequest(request * req);

	/************************************************************************/
	/* DOC ITEMS WRITEOFF                                                   */
	/************************************************************************/
	TResponseInfo * AddDocItemWriteOffRequest(request * req);
	TResponseInfo * SaveDocItemWriteOffRequest(request * req);
	TResponseInfo * RemoveDocItemWriteOffRequest(request * req);
	TResponseInfo * GetDocItemWriteOffRequest(request * req);
	TResponseInfo * GetDocItemsWriteOffViewRequest(request * req);
	TResponseInfo * ReserveDocItemWriteOffRequest(request * req);
	TResponseInfo * UnReserveDocItemWriteOffRequest(request * req);

	/************************************************************************/
	/* BANK ACCOUNTS                                                        */
	/************************************************************************/
	TResponseInfo * AddBankAccountRequest(request * req);
	TResponseInfo * SaveBankAccountRequest(request * req);
	TResponseInfo * RemoveBankAccountRequest(request * req);
	TResponseInfo * GetBankAccountRequest(request * req);
	TResponseInfo * GetBankAccountsRequest(request * req);
	TResponseInfo * GetBankAccountsViewRequest(request * req);
	TResponseInfo * GetBankAccountViewRequest(request * req);

	/************************************************************************/
	/* SUMMARY                                                              */
	/************************************************************************/
	TResponseInfo * CalculateProductSummaryRequest(request * req);

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	TResponseInfo * GetDocPaymentSumRequest(request * req);

	/************************************************************************/
	/* IN PAYMENT DOCUMENT                                                  */
	/************************************************************************/

	TResponseInfo * AddInPaymentDocumentRequest(request * req);
	TResponseInfo * SaveInPaymentDocumentRequest(request * req);
	TResponseInfo * RemoveInPaymentDocumentRequest(request * req);
	TResponseInfo * GetInPaymentDocumentRequest(request * req);
	TResponseInfo * GetInPaymentDocumentViewRequest(request * req);
	TResponseInfo * GetInPaymentDocumentsRequest(request * req);
	TResponseInfo * GetInPaymentDocumentsViewRequest(request * req);
	TResponseInfo * PostInPaymentDocumentRequest(request * req);
	TResponseInfo * UnpostInPaymentDocumentRequest(request * req);

	/************************************************************************/
	/* OUT PAYMENT DOCUMENT                                                  */
	/************************************************************************/

	TResponseInfo * AddOutPaymentDocumentRequest(request * req);
	TResponseInfo * SaveOutPaymentDocumentRequest(request * req);
	TResponseInfo * RemoveOutPaymentDocumentRequest(request * req);
	TResponseInfo * GetOutPaymentDocumentRequest(request * req);
	TResponseInfo * GetOutPaymentDocumentViewRequest(request * req);
	TResponseInfo * GetOutPaymentDocumentsRequest(request * req);
	TResponseInfo * GetOutPaymentDocumentsViewRequest(request * req);
	TResponseInfo * PostOutPaymentDocumentRequest(request * req);
	TResponseInfo * UnpostOutPaymentDocumentRequest(request * req);

	/************************************************************************/
	/* ADD COSTS  DOCUMENT                                                  */
	/************************************************************************/

	TResponseInfo * AddAddCostsDocumentRequest(request * req);
	TResponseInfo * SaveAddCostsDocumentRequest(request * req);
	TResponseInfo * RemoveAddCostsDocumentRequest(request * req);
	TResponseInfo * GetAddCostsDocumentRequest(request * req);
	TResponseInfo * GetAddCostsDocumentViewRequest(request * req);
	TResponseInfo * GetAddCostsDocumentsRequest(request * req);
	TResponseInfo * GetAddCostsDocumentsViewRequest(request * req);
	TResponseInfo * PostAddCostsDocumentRequest(request * req);
	TResponseInfo * UnpostAddCostsDocumentRequest(request * req);

	/************************************************************************/
	/* RELATION DOCUMENTS                                                   */
	/************************************************************************/
	TResponseInfo * GetRelationDocumentsRequest(request * req);


	/************************************************************************/
	/* ACCOUNT SUMMARY REQUEST                                              */
	/************************************************************************/
	TResponseInfo * GetAccountSummaryRequest(request * req);
	TResponseInfo * GetAccountSummariesViewRequest(request * req);
	TResponseInfo * GetTaxesSummaryViewRequest(request * req);
	/************************************************************************/
	/* DOC PRODUCT IN                                                       */
	/************************************************************************/

	TResponseInfo * AddProductInDocumentRequest(request * req);
	TResponseInfo * SaveProductInDocumentRequest(request * req);
	TResponseInfo * RemoveProductInDocumentRequest(request * req);
	TResponseInfo * GetProductInDocumentRequest(request * req);
	TResponseInfo * GetProductInDocumentViewRequest(request * req);
	TResponseInfo * GetProductInDocumentsRequest(request * req);
	TResponseInfo * GetProductInDocumentsViewRequest(request * req);
	TResponseInfo * PostProductInDocumentRequest(request * req);
	TResponseInfo * UnpostProductInDocumentRequest(request * req);

	/************************************************************************/
	/* DOC RETURN IN                                                        */
	/************************************************************************/
	TResponseInfo * AddReturnInDocumentRequest(request * req);
	TResponseInfo * SaveReturnInDocumentRequest(request * req);
	TResponseInfo * RemoveReturnInDocumentRequest(request * req);
	TResponseInfo * GetReturnInDocumentRequest(request * req);
	TResponseInfo * GetReturnInDocumentViewRequest(request * req);
	TResponseInfo * GetReturnInDocumentsRequest(request * req);
	TResponseInfo * GetReturnInDocumentsViewRequest(request * req);
	TResponseInfo * PostReturnInDocumentRequest(request * req);
	TResponseInfo * UnpostReturnInDocumentRequest(request * req);

	/************************************************************************/
	/* DOC ITEM RETURN  IN                                                  */
	/************************************************************************/

	TResponseInfo * SaveDocItemReturnInRequest(request * req);
	TResponseInfo * SaveDocItemsReturnInRequest(request * req);
	TResponseInfo * RemoveDocItemReturnInRequest(request * req);
	TResponseInfo * GetDocItemReturnInRequest(request * req);
	TResponseInfo * GetDocItemsReturnInViewRequest(request * req);

	/************************************************************************/
	/* DOC ITEM SERVICES                                                    */
	/************************************************************************/
	TResponseInfo * AddDocItemServiceRequest(request * req);
	TResponseInfo * SaveDocItemServiceRequest(request * req);
	TResponseInfo * RemoveDocItemServiceRequest(request * req);
	TResponseInfo * GetDocItemServiceRequest(request * req);

	/************************************************************************/
	/* DOC ITEM PRODUCT IN                                                  */
	/************************************************************************/

	TResponseInfo * SaveDocItemProductInRequest(request * req);
	TResponseInfo * SaveDocItemsProductInRequest(request * req);
	TResponseInfo * RemoveDocItemProductInRequest(request * req);
	TResponseInfo * GetDocItemProductInRequest(request * req);
	TResponseInfo * GetDocItemsProductInViewRequest(request * req);

	/************************************************************************/
	/* DOC PRODUCT OUT                                                      */
	/************************************************************************/

	TResponseInfo * AddProductOutDocumentRequest(request * req);
	TResponseInfo * SaveProductOutDocumentRequest(request * req);
	TResponseInfo * RemoveProductOutDocumentRequest(request * req);
	TResponseInfo * GetProductOutDocumentRequest(request * req);
	TResponseInfo * GetProductOutDocumentViewRequest(request * req);
	TResponseInfo * GetProductOutDocumentsRequest(request * req);
	TResponseInfo * GetProductOutDocumentsViewRequest(request * req);
	TResponseInfo * PostProductOutDocumentRequest(request * req);
	TResponseInfo * UnpostProductOutDocumentRequest(request * req);

	/************************************************************************/
	/* DOC ITEMS WRITEOFF                                                   */
	/************************************************************************/
	TResponseInfo * AddDocItemProductOutRequest(request * req);
	TResponseInfo * SaveDocItemProductOutRequest(request * req);
	TResponseInfo * RemoveDocItemProductOutRequest(request * req);
	TResponseInfo * GetDocItemProductOutRequest(request * req);
	TResponseInfo * GetDocItemsProductOutViewRequest(request * req);
	TResponseInfo * ReserveDocItemProductOutRequest(request * req);
	TResponseInfo * UnReserveDocItemProductOutRequest(request * req);

	/************************************************************************/
	/* DOC ITEM PRODUCT OUT SERVICES                                        */
	/************************************************************************/
	TResponseInfo * SaveDocItemProductOutServiceRequest(request * req);
	TResponseInfo * RemoveDocItemProductOutServiceRequest(request * req);
	TResponseInfo * GetDocItemProductOutServiceRequest(request * req);

	/************************************************************************/
	/* DOC RETURN  OUT                                                      */
	/************************************************************************/
	TResponseInfo * AddReturnOutDocumentRequest(request * req);
	TResponseInfo * SaveReturnOutDocumentRequest(request * req);
	TResponseInfo * RemoveReturnOutDocumentRequest(request * req);
	TResponseInfo * GetReturnOutDocumentRequest(request * req);
	TResponseInfo * GetReturnOutDocumentViewRequest(request * req);
	TResponseInfo * GetReturnOutDocumentsRequest(request * req);
	TResponseInfo * GetReturnOutDocumentsViewRequest(request * req);
	TResponseInfo * PostReturnOutDocumentRequest(request * req);
	TResponseInfo * UnpostReturnOutDocumentRequest(request * req);
	/************************************************************************/
	/* DOC ITEM RETURN OUT                                                  */
	/************************************************************************/
	TResponseInfo * AddDocItemReturnOutRequest(request * req);
	TResponseInfo * SaveDocItemReturnOutRequest(request * req);
	TResponseInfo * RemoveDocItemReturnOutRequest(request * req);
	TResponseInfo * GetDocItemReturnOutRequest(request * req);
	TResponseInfo * GetDocItemsReturnOutViewRequest(request * req);
	TResponseInfo * ReserveDocItemReturnOutRequest(request * req);
	TResponseInfo * UnReserveDocItemReturnOutRequest(request * req);

	/************************************************************************/
	/* DOC INVOICE                                                          */
	/************************************************************************/
	TResponseInfo * AddInvoiceDocumentRequest(request * req);
	TResponseInfo * SaveInvoiceDocumentRequest(request * req);
	TResponseInfo * RemoveInvoiceDocumentRequest(request * req);
	TResponseInfo * GetInvoiceDocumentRequest(request * req);
	TResponseInfo * GetInvoiceDocumentViewRequest(request * req);
	TResponseInfo * GetInvoiceDocumentsViewRequest(request * req);
	TResponseInfo * PostInvoiceDocumentRequest(request * req);
	TResponseInfo * UnpostInvoiceDocumentRequest(request * req);

	/************************************************************************/
	/* DOC ITEMS INVOICE                                                   */
	/************************************************************************/
	TResponseInfo * AddDocItemInvoiceRequest(request * req);
	TResponseInfo * SaveDocItemInvoiceRequest(request * req);
	TResponseInfo * RemoveDocItemInvoiceRequest(request * req);
	TResponseInfo * GetDocItemInvoiceRequest(request * req);
	TResponseInfo * GetDocItemsInvoiceViewRequest(request * req);
	TResponseInfo * ReserveDocItemInvoiceRequest(request * req);
	TResponseInfo * UnReserveDocItemInvoiceRequest(request * req);

	/************************************************************************/
	/* DOC ORDER IN                                                          */
	/************************************************************************/
	TResponseInfo * AddOrderInDocumentRequest(request * req);
	TResponseInfo * SaveOrderInDocumentRequest(request * req);
	TResponseInfo * RemoveOrderInDocumentRequest(request * req);
	TResponseInfo * GetOrderInDocumentRequest(request * req);
	TResponseInfo * GetOrderInDocumentViewRequest(request * req);
	TResponseInfo * GetOrderInDocumentsViewRequest(request * req);
	TResponseInfo * PostOrderInDocumentRequest(request * req);
	TResponseInfo * UnpostOrderInDocumentRequest(request * req);

	/************************************************************************/
	/* DOC ITEMS ORDER IN                                                   */
	/************************************************************************/
	TResponseInfo * AddDocItemOrderInRequest(request * req);
	TResponseInfo * SaveDocItemOrderInRequest(request * req);
	TResponseInfo * RemoveDocItemOrderInRequest(request * req);
	TResponseInfo * GetDocItemOrderInRequest(request * req);
	TResponseInfo * GetDocItemsOrderInViewRequest(request * req);

	/************************************************************************/
	/* DOC ORDER OUT                                                        */
	/************************************************************************/
	TResponseInfo * AddOrderOutDocumentRequest(request * req);
	TResponseInfo * SaveOrderOutDocumentRequest(request * req);
	TResponseInfo * RemoveOrderOutDocumentRequest(request * req);
	TResponseInfo * GetOrderOutDocumentRequest(request * req);
	TResponseInfo * GetOrderOutDocumentViewRequest(request * req);
	TResponseInfo * GetOrderOutDocumentsViewRequest(request * req);
	TResponseInfo * PostOrderOutDocumentRequest(request * req);
	TResponseInfo * UnpostOrderOutDocumentRequest(request * req);

	/************************************************************************/
	/* DOC ITEMS ORDER OUT                                                  */
	/************************************************************************/
	TResponseInfo * AddDocItemOrderOutRequest(request * req);
	TResponseInfo * SaveDocItemOrderOutRequest(request * req);
	TResponseInfo * RemoveDocItemOrderOutRequest(request * req);
	TResponseInfo * GetDocItemOrderOutRequest(request * req);
	TResponseInfo * GetDocItemsOrderOutViewRequest(request * req);

	/************************************************************************/
	/* DOC INVOICE  FACTURA                                                 */
	/************************************************************************/
	TResponseInfo * AddInvoiceFacturaDocumentRequest(request * req);
	TResponseInfo * SaveInvoiceFacturaDocumentRequest(request * req);
	TResponseInfo * RemoveInvoiceFacturaDocumentRequest(request * req);
	TResponseInfo * GetInvoiceFacturaDocumentRequest(request * req);
	TResponseInfo * GetInvoiceFacturaDocumentViewRequest(request * req);
	TResponseInfo * GetInvoiceFacturaDocumentsViewRequest(request * req);

	/************************************************************************/
	/* DOC ITEMS INVOICE  FACTURA                                           */
	/************************************************************************/
	TResponseInfo * AddDocItemInvoiceFacturaRequest(request * req);
	TResponseInfo * SaveDocItemInvoiceFacturaRequest(request * req);
	TResponseInfo * RemoveDocItemInvoiceFacturaRequest(request * req);
	TResponseInfo * GetDocItemInvoiceFacturaRequest(request * req);
	TResponseInfo * GetDocItemsInvoiceFacturaViewRequest(request * req);

	/************************************************************************/
	/* PRICE ANALYSIS                                                       */
	/************************************************************************/
	TResponseInfo * CalculatePaymentSumRequest(request * req);
	TResponseInfo * CalculateDocItemPriceRequest(request * req);
	TResponseInfo * GetPriceAnalyticRequest(request * req);
	TResponseInfo * GetPriceAnalyticsRequest(request * req);

	/************************************************************************/
	/* PRICE CATEGORY                                                       */
	/************************************************************************/
	TResponseInfo * SavePriceCategoryRequest(request * req);
	TResponseInfo * AddPriceCategoryRequest(request * req);
	TResponseInfo * RemovePriceCategoryRequest(request * req);
	TResponseInfo * GetPriceCategoryRequest(request * req);
	TResponseInfo * GetPriceCategoriesRequest(request * req);
	TResponseInfo * AddCustomPriceCategoriesRequest(request * req);
	TResponseInfo * CalculatePriceCategoriesViewRequest(request * req);
	TResponseInfo * GetPriceCategoriesViewRequest(request * req);

	/************************************************************************/
	/* PRODUCT SUMMARIES                                                    */
	/************************************************************************/
	TResponseInfo * GetProductSummariesViewRequest(request * req);
	TResponseInfo * GetProductSummariesSubsViewRequest(request * req);

	/************************************************************************/
	/* DUPLICATES                                                           */
	/************************************************************************/
	TResponseInfo * DuplicateProductRequest(request * req);
	TResponseInfo * DuplicatePartnerRequest(request * req);
	TResponseInfo * DuplicateDocumentRequest(request * req);

	/************************************************************************/
	/* WAREHOUSE REPORTS                                                    */
	/************************************************************************/
	TResponseInfo * GetReportShortIncomeRequest(request * req);
	TResponseInfo * GetReportDetailedIncomeRequest(request * req);
	TResponseInfo * GetReportShortSalesRequest(request * req);
	TResponseInfo * GetReportBalancesAtWarehouseRequest(request * req);
	TResponseInfo * GetReportProductMoveRequest(request * req);
	TResponseInfo * GetReportTurnoverRequest(request * req);
	TResponseInfo * GetReportProductTurnoverRequest(request * req);
	TResponseInfo * GetReportMinProductBalanceRequest(request * req);
	TResponseInfo * GetReportSalesWithProfitRequest(request * req);

	/************************************************************************/
	/*  REPORTS TEMPLATE                                                    */
	/************************************************************************/
	TResponseInfo * AddReportTemplateRequest(request * req);
	TResponseInfo * SaveReportTemplateRequest(request * req);
	TResponseInfo * RemoveReportTemplateRequest(request * req);
	TResponseInfo * GetReportTemplateRequest(request * req);
	TResponseInfo * GetReportTemplatesRequest(request * req);

	/************************************************************************/
	/* REPORTS                                                              */
	/************************************************************************/
	TResponseInfo * GetReportsRequest(request * req);
	TResponseInfo * GetReportDataSourcesRequest(request * req);
	TResponseInfo * GetReportClassPropertiesRequest(request * req);

	/************************************************************************/
	/* REPORTS DATA															*/
	/************************************************************************/
	TResponseInfo * GetReportDataRequest(request * req);

	/************************************************************************/
	/* PRINT XML                                                            */
	/************************************************************************/
	TResponseInfo * GetDocProductInPrintRequest(request * req);
	TResponseInfo * GetDocProductOutPrintRequest(request * req);
	TResponseInfo * GetDocInvoicePrintRequest(request * req);
	TResponseInfo * GetDocOrderInPrintRequest(request * req);
	TResponseInfo * GetDocOrderOutPrintRequest(request * req);

	/************************************************************************/
	/* PDF                                                                  */
	/************************************************************************/
	TResponseInfo * GetReportPDFRequest(request * req);

	/************************************************************************/
	/* GET CALCULATE LOTS                                                   */
	/************************************************************************/
	TResponseInfo * GetCalculateLotsRequest(request * req);

	/************************************************************************/
	/*  HELP TOPICS 		                                                */
	/************************************************************************/
	TResponseInfo * AddHelpTopicRequest(request * req);
	TResponseInfo * SaveHelpTopicRequest(request * req);
	TResponseInfo * RemoveHelpTopicRequest(request * req);
	TResponseInfo * GetHelpTopicRequest(request * req);
	TResponseInfo * GetHelpTopicsRequest(request * req);
	TResponseInfo * ChangeHelpTopicPositionRequest(request * req);

	//
	TResponseInfo * RecalculateProductsPriceRequest(request * req);

	/************************************************************************/
	/* PRODUCTS EXEL                                                        */
	/************************************************************************/
	TResponseInfo * ImportProductsCSVRequest(request * req);

	/************************************************************************/
	/* RELATION                                                             */
	/************************************************************************/
	TResponseInfo * AddRelationRequest(request * req);
	TResponseInfo * GetRelationRequest(request * req);

	/************************************************************************/
	/* PERMISSIONS                                                          */
	/************************************************************************/
	TResponseInfo * CheckPermissionRequest(request * req);
	TResponseInfo * AddCustomPermissionsRequest(request * req);
	TResponseInfo * GetPermissionsViewRequest(request * req);
	TResponseInfo * GetPermissionViewRequest(request * req);
	TResponseInfo * GetPermissionRequest(request * req);

	/************************************************************************/
	/* CHECK SERVER STATUS                                                  */
	/************************************************************************/
	TResponseInfo * CheckServerStatusRequest(request * req);
	TResponseInfo * StopServerRequest(request * req);
	TResponseInfo * GetServerInfoRequest(request * req);

	/************************************************************************/
	/* GRID TO EXCEL                                                        */
	/************************************************************************/
	TResponseInfo * ExportGridToExcelRequest(request * req);

	/************************************************************************/
	/* Files                                                                */
	/************************************************************************/
	TResponseInfo * GetFileRequest(request * req);

	/************************************************************************/
	/* SERVER TIME(DATE)                                                    */
	/************************************************************************/
	TResponseInfo * GetDateTimeRequest(request * req);

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	string ParseErrorCode(string error);
	vector<int> ParseVectorIds(string params);
	vector<int> ParseVectorIdsJson(string params);
	vector<int> ParseVectorIdsXml(string params);

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/

	int GetPermissionIdFromCommand(string command);

	/************************************************************************/
	/* MESSAGES                                                             */
	/************************************************************************/
	TResponseInfo * SaveMessageRequest(request * req);
	TResponseInfo * AddMessageRequest(request * req);
	TResponseInfo * RemoveMessageRequest(request * req);
	TResponseInfo * GetMessageRequest(request * req);
	TResponseInfo * GetMessagesRequest(request * req);
	TResponseInfo * GetMessageViewRequest(request * req);
	TResponseInfo * GetMessagesViewRequest(request * req);

	/************************************************************************/
	/* MAKE DOCUMENT INVOICE                                                */
	/************************************************************************/
	TResponseInfo * MakeDocumentInvoiceRequest(request * req);

	/************************************************************************/
	/* PROFIT DIAGRAM                                                       */
	/************************************************************************/
	TResponseInfo * GetFinProfitSummariesRequest(request * req);
	TResponseInfo * GetFinProfitPaymentsSummariesRequest(request * req);
	TResponseInfo * GetFinSummariesViewRequest(request * req);
	TResponseInfo * GetAssetsRequest(request * req);

	TResponseInfo * AddWebstoreRequest(request * req);
	TResponseInfo * RemoveWebstoreRequest(request * req);
	TResponseInfo * GetWebstoreRequest(request * req);
	TResponseInfo * GetWebstoresRequest(request * req);
	TResponseInfo * SaveWebstoreRequest(request * req);

	/************************************************************************/
	/* GET PHP                                                              */
	/************************************************************************/
	 TResponseInfo * GetPHP(request * req);

	 /************************************************************************/
	 /* CRONTASKS                                                            */
	 /************************************************************************/
	 TResponseInfo * GetCronTasksViewRequest(request * req);

};
#endif
