#include <opal/manager.h>
#include <opal/pcss.h>
#include <sip/sipep.h>
#include <opal/opalmixer.h>
#include <iostream>

class MyPCSSEndPoint : public OpalPCSSEndPoint {
	PCLASSINFO(MyPCSSEndPoint, OpalPCSSEndPoint);
	public:
		MyPCSSEndPoint(OpalManager & manager);
		virtual ~MyPCSSEndPoint();
		virtual PBoolean OnShowIncoming(const OpalPCSSConnection & connection);
		virtual PBoolean OnShowOutgoing(const OpalPCSSConnection & connection);
};

MyPCSSEndPoint::MyPCSSEndPoint(OpalManager & manager) : OpalPCSSEndPoint(manager) {
	cout << "MyPCSSEndPoint constructed." << endl;
}
MyPCSSEndPoint::~MyPCSSEndPoint() {
	cout << "MyPCSSEndPoint destroyed." << endl;
	PThread::Sleep(100);
}

PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection) {
	return AcceptIncomingCall(connection.GetToken());
}

PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection) {
	return PTrue;
}

class MySIPEndPoint : public SIPEndPoint {
	PCLASSINFO(MySIPEndPoint, SIPEndPoint);
	public:
		MySIPEndPoint(OpalManager & manager);
		virtual ~MySIPEndPoint();
		void OnRegistrationStatus(const RegistrationStatus & status);
};

MySIPEndPoint::MySIPEndPoint(OpalManager & manager) : SIPEndPoint(manager) {
	cout << "MySIPEndPoint constructed." << endl;
}

MySIPEndPoint::~MySIPEndPoint() {
	cout << "MySIPEndPoint destroyed." << endl;
	PThread::Sleep(100);
}

void MySIPEndPoint::OnRegistrationStatus(const RegistrationStatus & status) {
	std::cout << "   *** New SIP Endpoint status is " << SIP_PDU::GetStatusCodeDescription(status.m_reason) << std::endl;
	SIPEndPoint::OnRegistrationStatus(status);
}

class MyManager : public OpalManager {
	PCLASSINFO(MyManager, OpalManager);
	public:
		MyManager();
		~MyManager();
		void OnConnected(OpalConnection & connection);
		void OnAlerting(OpalConnection & connection);
		void OnNewConnection(OpalConnection & connection);
		void OnReleased(OpalConnection & connection);
};

MyManager::MyManager(): OpalManager() {
	cout << "MyManager created." << endl;
}

MyManager::~MyManager() {
	cout << "MyManager destroyed." << endl;
	PThread::Sleep(100);
}

void MyManager::OnConnected(OpalConnection & connection) {
	std::cout << "   *** MyManager.OnConnected()" << std::endl;
	OpalManager::OnConnected(connection);
}

void MyManager::OnAlerting(OpalConnection & connection) {
	std::cout << "   *** MyManager.OnAlerting()" << std::endl;
	OpalManager::OnAlerting(connection);
}

void MyManager::OnNewConnection(OpalConnection & connection) {
	std::cout << "   *** MyManager.OnNetConnection()" << std::endl;
	OpalManager::OnNewConnection(connection);
}

void MyManager::OnReleased(OpalConnection & connection) {
	std::cout << "   *** MyManager.OnReleased()" << std::endl;
	OpalManager::OnReleased(connection);
}

class MyProcess: public PProcess {
	//This macro defines some necessary functions
	//See: http://www.opalvoip.org/docs/ptlib-v2_8/d6/d1e/object_8h.html
	PCLASSINFO(MyProcess, PProcess);
	public:
		MyProcess();
		void Main();
};

MyProcess::MyProcess() : PProcess("Test") { }

/* PCREATE_PROCESS macro:
 * 1. Defines the main() function.
 * 2. Creates an instance of MyProcess.
 * 3. Calls instance->PreInitialise() which is inherited from PProcess.
 * 4. Calls instance->InternalMain() which id inherited from PPrcoess
 *    instance->InternalMain() calls instance->Main() which we define.
 */
PCREATE_PROCESS(MyProcess)

void MyProcess::Main() {
	PString aor;
	PString REGISTRAR_ADDRESS = "ekiga.net";
	PString STUN_SERVER_ADDRESS = "stun.ekiga.net";
	PString USER_NAME;
	PString PASSWORD;
	PString SIP_ADDRESS = "sip:500@ekiga.net";
	//PString SIP_ADDRESS = "sip:*0131800xxxxxxx@ekiga.net";

	if (USER_NAME.GetLength() == 0 || PASSWORD.GetLength() == 0) {
		std::cout << "Please set your username and password in the code." << std::endl;
		return;
	}

	PTrace::Initialise(5, "log");

	MyManager manager = MyManager();
	// [Tom] TODO: Research why the NAT type is important.
	PSTUNClient::NatTypes nat = manager.SetSTUNServer(STUN_SERVER_ADDRESS);
	std::cout << "NAT type: " << nat << std::endl;

	MyPCSSEndPoint * soundEp = new MyPCSSEndPoint(manager);
	std::cout << "Sound output:" << std::endl;
	std::cout << "===================================================================" << std::endl;
	std::cout << soundEp->GetSoundChannelPlayDevice() << std::endl << std::endl;
	std::cout << "Media formats:" << std::endl;
	std::cout << "===================================================================" << std::endl;
	std::cout << soundEp->GetMediaFormats() << std::endl << std::endl;
	std::cout << "Registered formats:" << std::endl;
	std::cout << "===================================================================" << std::endl;
	std::cout << OpalMediaFormat::GetAllRegisteredMediaFormats() << std::endl << std::endl;

	MySIPEndPoint * endPoint = new MySIPEndPoint(manager);
	endPoint->SetDefaultLocalPartyName(USER_NAME);
	endPoint->StartListeners(endPoint->GetDefaultListeners());
	SIPRegister::Params params;
	params.m_registrarAddress = REGISTRAR_ADDRESS;
	params.m_addressOfRecord = USER_NAME;
	params.m_password = PASSWORD;
	std::cout << "Registering with " << params.m_registrarAddress << "; this may take a while..." << std::endl;
	endPoint->Register(params, aor);

	OpalMixerEndPoint * mixerEP = new OpalMixerEndPoint(manager, "mcu");
	OpalMixerNodeInfo * mcuNodeInfo = new OpalMixerNodeInfo;
	mcuNodeInfo->m_name = "Telekarma";
	mixerEP->SetAdHocNodeInfo(mcuNodeInfo);
	mixerEP->StartListeners(mixerEP->GetDefaultListeners());
	int i;
	for(i = 20; i > 0; i--) {
		if (endPoint->IsRegistered(aor)) {
			break;
		}
		PThread::Sleep(1000);
	}

	if (i > 0) {
		std::cout << "Registration succeeded; aor=" << aor << std::endl;
		std::cout << "Calling..." << std::endl;
		PSafePtr<OpalCall> call2 = manager.SetUpCall("mcu:*", "pc:*");
		PSafePtr<OpalCall> call = manager.SetUpCall("mcu:*", SIP_ADDRESS);
		while(!call);
//		manager.StartRecording("test.wav");
		std::cout << "Connection will automatically terminate after 15 seconds..." << std::endl;
		PThread::Sleep(15*1000);
//		manager.StopRecording();
		std::cout << "Connection automatically terminated by design." << std::endl;
		call->Clear();
		endPoint->Unregister(aor);
		while(endPoint->IsRegistered(aor));
		// [MV] maybe more to the exit/cleanup than this...
	} else {
		std::cout << "Registration attempt timed out." << std::endl;
	}
	std::cout << "Proof of concept main execution complete." << std::endl;
}
