/*
 * sipcmd, main.cpp
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA  02110-1301, USA.
 *
 */

#include <iostream>
#include <sstream>
#include <signal.h>
#include "main.h"
#include "commands.h"
#include "state.h"

//#include "channels.h"
int DIAL_TIMEOUT;

TPState *TPState::instance = NULL;

PCREATE_PROCESS(TestProcess);

static std::string stringify(const PString &broken) {
    std::stringstream s;
    s << broken;
    return s.str();
}

static void print_help() {
    cerr << "sipcmd options: " << endl
        << "-T <timeout> --dialtimeout <timeout>  dial timeout in seconds" << endl
        << "-u <name>    --user <name>            username (required)" << endl
        << "-c <passw>   --password <passw>       password for registration" << endl
        << "-l <addr>    --localaddress <addr>    local address to listen on" << endl 
        << "-o <file>    --opallog <file>         enable extra opal library logging to file" << endl
        << "-p <port>    --listenport <port>      the port to listen on" << endl 
        << "-P <proto>   -- protocol <proto>      sip/h323/rtp (required)" << endl 
        << "-r <nmbr>    --remoteparty <nmbr>     the party to call to" << endl 
        << "-x <prog>    --execute <prog>         program to follow" << endl  
        << "-d <prfx>    --audio-prefix <prfx>    recorded audio filename prefix" << endl 
        << "-f <file>    --file <file>            the name of played sound file" << endl 
        << "-g <addr>    --gatekeeper <addr>      gatekeeper to use" << endl 
        << "-w <addr>    --gateway <addr>         gateway to use" << endl 
        << "-a <name>    --alias <name>           username alias" << endl 
        << "-m <codec>   --mediaformat <codec>    select codec" << endl << endl;

    cerr << "The EBNF definition of the program syntax:" << endl
        << "<prog>  := cmd ';' <prog> | " << endl
        << "cmd     := call | answer | hangup" << endl
        << "           | dtmf | voice | record | wait" << endl
        << "           | setlabel | loop" << endl
        << "call    := 'c' remoteparty" << endl
        << "answer  := 'a' [ expectedremoteparty ]" << endl
        << "hangup  := 'h'" << endl
        << "dtmf    := 'd' digits" << endl
        << "voice   := 'v' audiofile" << endl
        << "record  := 'r' [ append ] [ silence ] [ iter ] millis audiofile" 
        << endl
        << "append  := 'a'" << endl
        << "silence := 's'" << endl
        << "closed  := 'c'" << endl
        << "iter    := 'i'" << endl
        << "activity:= 'a'" << endl
        << "wait    := 'w' [ activity | silence ] [ closed ] millis" << endl
        << "setlabel:= 'l' label" << endl
        << "loop    := 'j' [ how-many-times ] [ 'l' label ]" << endl;

    cerr << endl << "Example:" << endl
        << "\"c333;ws3000;d123;w200;lthrice;ws1000;vaudio;rsi4000f.out;" 
        << "j3lthrice;h;j4\"" << endl
        << "parses to: " << endl
        << "1. do this four times:" << endl
        << "1.1. call to 333" << endl
        << "1.2. wait until silent (max 3000 ms)" << endl
        << "1.3. send digits 1231.4. wait 200 ms" << endl
        << "1.5. do this three times:" << endl
        << "1.5.1. wait until silent (max 1000 ms)" << endl
        << "1.5.2. send voice file 'audio'"
        << "1.5.3. record until silent (max 4000 ms) to files 'f-'<0-3>'-'<0-2>'.out'" << endl
        << "1.6. hangup" << endl
        << "1.7. wait 2000 ms" << endl;
}

std::string get_call_end_reason_string(OpalConnection::CallEndReason r) {
    switch (r) {
        case OpalConnection::EndedByLocalUser:
            return "EndedByLocalUser";
        case OpalConnection::EndedByNoAccept:
            return "EndedByNoAccept";
        case OpalConnection::EndedByAnswerDenied:
            return "EndedByAnswerDenied";
        case OpalConnection::EndedByRemoteUser:
            return "EndedByRemoteUser";
        case OpalConnection::EndedByRefusal:
            return "EndedByRefusal";
        case OpalConnection::EndedByNoAnswer:
            return "EndedByNoAnswer";
        case OpalConnection::EndedByCallerAbort:
            return "EndedByCallerAbort";
        case OpalConnection::EndedByTransportFail:
            return "EndedByTransportFail";
        case OpalConnection::EndedByConnectFail:
            return "EndedByConnectFail";
        case OpalConnection::EndedByGatekeeper:
            return "EndedByGateKeeper";
        case OpalConnection::EndedByNoUser:
            return "EndedByNoUser";
        case OpalConnection::EndedByNoBandwidth:
            return "EndedByNoBandwidth";
        case OpalConnection::EndedByCapabilityExchange:
            return "EndedByCapabilityExchange";
        case OpalConnection::EndedByCallForwarded:
            return "EndedByCallForwarded";
        case OpalConnection::EndedBySecurityDenial:
            return "EndedBySecurityDenial";
        case OpalConnection::EndedByLocalBusy:
            return "EndedByLocalBusy";
        case OpalConnection::EndedByLocalCongestion:
            return "EndedByLocalCongestion";
        case OpalConnection::EndedByRemoteBusy:
            return "EndedByRemoteBusy";
        case OpalConnection::EndedByRemoteCongestion:
            return "EndedByRemoteCongestion";
        case OpalConnection::EndedByUnreachable:
            return "EndedByUnreachable";
        case OpalConnection::EndedByNoEndPoint:
            return "EndedByNoEndPoint";
        case OpalConnection::EndedByHostOffline:
            return "EndedByHostOffline";
        case OpalConnection::EndedByTemporaryFailure:
            return "EndedByTemporaryFailure";
        case OpalConnection::EndedByQ931Cause:
            return "EndedByQ931Cause";
        case OpalConnection::EndedByDurationLimit:
            return "EndedByDurationLimit";
        case OpalConnection::EndedByInvalidConferenceID:
            return "EndedByInvalidConferenceID";
        default:
            break;
    }

    return "Unknown";
}

void signalHandler(int sig) {
    std::cerr << "signal caught!" << std::endl;
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            TPState::Instance().SetState(TPState::TERMINATED);
        default:
            break;
    }
}

void initSignalHandling() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = signalHandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

TestProcess::TestProcess() :
    PProcess("LoxBerry Text2SIP", "text2sip")
{
}

void TestProcess::Main()
{
    std::cout << "Starting sipcmd LoxBerry Text2SIP Plugin Edition v0.7a adapted by C.Woerstenfeld (sipcmd developed by Tuomo Makkonen)" << std::endl;
//  debug << "in debug mode" << std::endl;
    PArgList &args = GetArguments();

    initSignalHandling();
    manager = new Manager();
    if (manager->Init(args))
        manager->Main(args);

    std::cout << "Exiting." << std::endl;
    delete manager;

}

LocalEndPoint::LocalEndPoint(Manager &m) :
    OpalLocalEndPoint(m), m_manager(m)
{
  std::cerr << "Created LocalEndPoint" << std::endl;
}

PSafePtr<OpalConnection> LocalEndPoint::MakeConnection(OpalCall &call, 
        const PString &remoteParty, void *userData, unsigned int options,
        OpalConnection::StringOptions *opts)
{
    std::cerr << "LocalEndpoint::" << __func__ << std::endl;
    return AddConnection(CreateConnection(call, userData, options, opts));
// return OpalLocalEndPoint::MakeConnection(call, remoteParty, userData, options, opts);
}

OpalLocalConnection * LocalEndPoint::CreateConnection(
        OpalCall & call, void *userData, unsigned opts, 
        OpalConnection::StringOptions *stropts)
{
    std::cerr << "LocalEndpoint" << __func__ << std::endl;
    //return OpalLocalEndPoint::CreateConnection(call, userData);
    return new LocalConnection(call, *this, userData, opts, stropts);
}

// callback to hande data to be sent.
bool LocalEndPoint::OnReadMediaData(
    const OpalLocalConnection &connection,
    const OpalMediaStream &mediaStream,
    void *data, PINDEX size, PINDEX &length) 
{
  std::cerr << __func__ << " datalen="<< size << std::endl;
  return true;

  //return const_cast<OpalMediaStream*>(&mediaStream)->ReadData(
  //       (BYTE*)data, size, length);
}

// callback to handle received data.
bool LocalEndPoint::OnWriteMediaData(
    const OpalLocalConnection &connection,
    const OpalMediaStream &mediaStream,
    const void *data, PINDEX length, PINDEX &written)
{
  std::cerr << __func__ << std::endl;
  return true;
  //return const_cast<OpalMediaStream*>(&mediaStream)->WriteData(
  //        (BYTE*)data, length, written);
}

Manager::Manager() : localep(NULL), sipep(NULL), h323ep(NULL), listenmode(false), listenerup(false), pauseBeforeDialing(false), mediaFilter("*")
{
  std::cerr << __func__  << std::endl;
}


Manager:: ~Manager()
{
  std::cerr << __func__ << std::endl;
  delete (m_rtpsession);
}


void Manager::Main(PArgList &args)
{
  std::cerr << __func__ << std::endl;


    // silence detection
    OpalSilenceDetector::Params sd = GetSilenceDetectParams();
    sd.m_mode = OpalSilenceDetector::AdaptiveSilenceDetection;
    SetSilenceDetectParams(sd);

    if (pauseBeforeDialing) {
        std::cerr << "sleep 1000 ms to allow time for registration ... " << std::endl; 
        PThread::Sleep(1000);
        std::cerr << "Done!" << std::endl;
    }

    // init command sequence
    const char *cmdseq = "";
    std::vector<Command*> sequence;
    
    if (args.HasOption('T')) {
               
        DIAL_TIMEOUT = args.GetOptionString('T').AsInteger();
        cerr << "DIAL_TIMEOUT is "<< DIAL_TIMEOUT << endl;
    }

    if (args.HasOption('x')) {
        
        cmdseq = args.GetOptionString('x');
    }

    if (args.HasOption('m')) {
         
        mediaFilter = stringify(args.GetOptionString('m'));
        std::cerr << "Codec-Filter: " << mediaFilter <<std::endl;
    }
 
    // Parse command sequence
    if(!Command::Parse(cmdseq, sequence)) {

        cerr << "Problem parsing command string \""
            << args.GetOptionString('x') << "\":" << endl
            << "  " << Command::GetErrorString() << endl;

        Command::DeleteSequence(sequence);
        return;
    }

    // run it
    else if(!Command::Run(sequence)) {

        cerr << "Problem running command sequence (\""
            << args.GetOptionString('x') << "\"):" << endl
            << "  " << Command::GetErrorString() << endl;

        Command::DeleteSequence(sequence);
    }

    std::cerr << "TestPhone::Main: shutting down" << endl;
    TPState::Instance().SetState(TPState::TERMINATED);
    ClearAllCalls();
    std::cerr << "TestPhone::Main: exiting..." << endl;
}

bool Manager::Init(PArgList &args)
{
    std::cerr << __func__ << std::endl;
    // Parse various command line arguments
    args.Parse(
            "T-dialtimeout:"
            "u-user:"
            "c-password:"
            "l-localaddress:"
            "o-opallog:"
            "p-listenport:"
            "P-protocol:"
            "R-register:"
            "x-execute:"
            "f-file:"
            "g-gatekeeper:"
            "w-gateway:"
            "h-help:"
            "a-alias:"
            "m-mediaformat:"
            );


    if (args.HasOption('h')) { 
        print_help();
        return false;
    }

    // enable opal logging if requested.
    if (args.HasOption('o'))
      PTrace::Initialise(5, args.GetOptionString('o'));

    if (!args.HasOption('P')) {
        std::cerr << "please define a protocol to use!" << std::endl;
        return false;
    }
     
    if (args.HasOption('p')) {
        TPState::Instance().SetListenPort(
                args.GetOptionString('p').AsInteger());
    }

    if (args.HasOption('l')) {
        TPState::Instance().SetLocalAddress(args.GetOptionString('l'));
    }

    string protocol = stringify(args.GetOptionString('P')); 
    if (!protocol.compare("sip")) {
        std::cerr << "initialising SIP endpoint..." << endl;
        sipep = new SIPEndPoint(*this);

        sipep->SetRetryTimeouts(10000, 30000);
        sipep->SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
        // AddRouteEntry("pc:.* = sip:<da>");
        // AddRouteEntry("sip:.* = pc:<db>");
        AddRouteEntry("local:.* = sip:<da>");
        AddRouteEntry("sip:.* = local:<db>");

        if (args.HasOption('u')) {
            sipep->SetDefaultLocalPartyName(args.GetOptionString('u'));
        }

       if (args.HasOption('a')) {
            sipep->SetDefaultDisplayName(args.GetOptionString('a'));
       }

        if (args.HasOption('c')) {
            SIPRegister::Params param;
            param.m_registrarAddress = args.GetOptionString('w');
            param.m_addressOfRecord = args.GetOptionString('u');
            param.m_password = args.GetOptionString('c');
            param.m_realm = args.GetOptionString('g');

            PString *aor = new PString("");
            //sipep->SetProxy(args.GetOptionString('w'));

	    if (!StartListener()) { 
	      return false;
	    }

            if (!sipep->Register(param, *aor)) { 
                std::cerr 
                    << "Could not register to " 
                    << param.m_registrarAddress << endl;
                return false;
            }
            else {
                std::cerr 
                    << "registered as " 
                    << aor->GetPointer(aor->GetSize()) << endl;
            }
        }

        TPState::Instance().SetProtocol(TPState::SIP);

    } else if (!protocol.compare("h323")) {
        std::cerr << "initialising H.323 endpoint..." << endl;
        h323ep = new H323EndPoint(*this);
        AddRouteEntry("pc:.*             = h323:<da>");
        AddRouteEntry("h323:.* = pc:<da>");
        if (args.HasOption('u')) {
            h323ep->SetDefaultLocalPartyName(args.GetOptionString('u'));
        }

        TPState::Instance().SetProtocol(TPState::H323);

    } else if (!protocol.compare("rtp")) {
        std::cerr << "initialising RTP endpoint..." << endl;
        TPState::Instance().SetProtocol(TPState::RTP);

    } else {
        std::cerr << "invalid protocol" << std::endl;
        return false;
    }
    
    if (args.HasOption('w')) {
        std::string val = args.GetOptionString('w');
        TPState::Instance().SetGateway(val);
    }

    SetAudioJitterDelay(20, 1000);
    DisableDetectInBandDTMF(true);

    localep = new LocalEndPoint(*this);

    TPState::Instance().SetManager(this);
    return true;
}

bool Manager::SendDTMF(const PString &dtmf)
{
    PSafePtr<OpalCall> call = FindCallWithLock(currentCallToken);
    if (!call) {
        std::cerr << "no call found with token="
            << currentCallToken << std::endl;
        return false;
    }

    bool ok = false;
    PSafePtr<OpalConnection> connection = call->GetConnection(
            listenmode ? 0 : 1);
    if (connection) {
        int i = 0;
        for (; i < dtmf.GetSize() - 1; i++) {
            if (!connection->SendUserInputTone(dtmf[i], 0))
                break;
            else {
                // sleep a while
                std::cout << "sent DTMF: [" << dtmf[i] << "]"  << std::endl;

                struct timespec tp;
                tp.tv_sec = 0;
                tp.tv_nsec = 500 * 1000 * 1000; // half a second
                nanosleep (&tp, 0);
            }
        }
        ok = (i == dtmf.GetSize() - 1 ? true : false);
    }

    if (!ok)
        std::cerr << "dtmf sending failed\n" << std::endl;

    return ok;
}
            
RTPSession::RTPSession(const Params& options) : RTP_UDP(options), m_audioformat(NULL)
{
  std::cerr << "RTP session created" << std::endl;
}

void RTPSession::SelectAudioFormat(const Payload payload) 
{
  if (m_audioformat) 
    delete m_audioformat;

  switch(payload) {
    case PCM16:
      m_audioformat = new OpalAudioFormat(
          "OPAL_PCM16", RTP_DataFrame::MaxPayloadType, "", 16, 8, 240, 0, 256, 8000, 0);
      std::cerr << "Payload format: OPAL_PCM16" << endl;
      break;
    case G711_ULAW:
      m_audioformat = new OpalAudioFormat(
          "OPAL_G711_ULAW_64K", RTP_DataFrame::PCMU, "PCMU", 16, 8, 240, 0, 256, 8000, 0);
      std::cerr << "Payload format: OPAL_G711_ULAW_64K" << endl;
      break;
    case G711_ALAW:
      m_audioformat = new OpalAudioFormat(
          "OPAL_G711_ALAW_64K", RTP_DataFrame::PCMA, "PCMA", 16, 8, 240, 0, 256, 8000, 0);
      std::cerr << "Payload format: OPAL_G711_ULAW_64K" << endl;
      break;
  }
}

RTP_Session::SendReceiveStatus RTPSession::OnReceiveData(RTP_DataFrame &frame) 
{
  SendReceiveStatus ret =  RTP_UDP::Internal_OnReceiveData(frame);
#if 1 // master dump
  std::ostringstream os;
  frame.PrintOn(os);
  std::cerr << os.str() << std::endl;
#endif

  TPState::Instance().GetRecordAudio().RecordFromBuffer(
      (const char*)frame.GetPayloadPtr(), frame.GetPayloadSize(), true);
  return ret;
}

RTP_Session::SendReceiveStatus RTPSession::OnSendData(RTP_DataFrame &frame) 
{
  SendReceiveStatus ret = RTP_UDP::Internal_OnSendData(frame);

#if 1 // master dump
  std::ostringstream os;
  frame.PrintOn(os);
  std::cerr << os.str() << std::endl;
#endif

  return ret;
}


RTP_Session::SendReceiveStatus RTPSession::OnReadTimeout(RTP_DataFrame &frame) {
  std::cerr << __func__ << std::endl;
  TPState::Instance().GetRecordAudio().StopRecording(false);
  return RTP_UDP::OnReadTimeout(frame);
}


bool Manager::MakeCall(const PString &remoteParty)
{
    std::cerr << "Setting up a call to: " << remoteParty << endl;
    PString token;
    if (TPState::Instance().GetProtocol() != TPState::RTP) {
      if (!SetUpCall("local:*", remoteParty, token)) {
        cerr << "Call setup to " << remoteParty << " failed" << endl;
        return false;
      }
    } else {
      // setting up rtp, split desination into ip and port parts
      PStringArray arr = remoteParty.Tokenise(":");
      if (arr.GetSize() != 2) {
        std::cerr << "invalid address: " << remoteParty << std::endl;
        return false;
      }
    
      // create rtp session
      RTP_Session::Params p;
      p.id = OpalMediaType::Audio().GetDefinition()->GetDefaultSessionId();
      p.encoding = OpalMediaType::Audio().GetDefinition()->GetRTPEncoding();
      p.userData = new RTPUserData;

      //m_rtpsession->SetUserData(new RTPUserData);
      m_rtpsession = new RTPSession(p);
      m_rtpsession->SelectAudioFormat(RTPSession::PCM16);

      // local and remote addresses
      PIPSocket::Address remote(arr[0]);
      PIPSocket::Address local(TPState::Instance().GetLocalAddress());
      
      if (!m_rtpsession->SetRemoteSocketInfo(remote, arr[1].AsInteger(), true)) {
        cerr << "could not set remote socket info" << endl;
        return false;
      }

      if (!m_rtpsession->Open(local,
            TPState::Instance().GetListenPort(), 
            TPState::Instance().GetListenPort(), 2)) {
        cerr << "could not open rtp session" << endl;
        return false;
      }

      m_rtpsession->SetJitterBufferSize(100, 1000);
      std::cerr  
         << "RTP local address:     " << local << std::endl
         << "RTP local data port:   " << m_rtpsession->GetLocalDataPort() << std::endl
         << "RTP remote address:    " << remote << std::endl
         << "RTP remote data port:  " << m_rtpsession->GetRemoteDataPort() << std::endl;
     
      std::cerr << "RTP stream set up!" << std::endl;
      TPState::Instance().SetState(TPState::ESTABLISHED);
      return true;
    }

    std::cerr << "connection set up to " << remoteParty << endl;
    std::string val = token;
    currentCallToken = val;
    return true;
}

unsigned Manager::CalculateTimestamp(const size_t size) 
{
  unsigned frametime = m_rtpsession->GetAudioFormat().GetFrameTime();
  unsigned framesize = m_rtpsession->GetAudioFormat().GetFrameSize();
  if (framesize == 0) 
    return frametime;

  unsigned frames = (size + framesize - 1) / framesize;
  return frames * frametime;
}

bool Manager::WriteFrame(RTP_DataFrame &frame) 
{
  return m_rtpsession && m_rtpsession->Internal_WriteData(frame);
}

bool Manager::ReadFrame(RTP_DataFrame &frame)
{
  return m_rtpsession && m_rtpsession->ReadBufferedData(frame);
}

bool Manager::StartListener()
{
    // TODO h323.
    PIPSocket::Address sipaddr = INADDR_ANY;
    std::cerr << "Listening for SIP signalling on " << sipaddr << ":" 
         << TPState::Instance().GetListenPort() << endl;
  
    OpalListenerUDP *siplistener = new OpalListenerUDP(
            *sipep, sipaddr, 
            TPState::Instance().GetListenPort());

    if (!siplistener) {
        std::cerr << "SIP listener creation failed!" << std::endl;
        return false;
    }
    if (!sipep->StartListener(siplistener)) { 
        std::cerr << "StartListener failed!" << std::endl;
        return false;
    }

    std::cerr << "SIP listener up" << std::endl;
    listenerup= true;
    return true;
}

bool Manager::OnOpenMediaStream(OpalConnection &connection, 
        OpalMediaStream &stream)
{
    std::cerr <<  __func__ << std::endl;
    if (!OpalManager::OnOpenMediaStream(connection, stream)) {
        std::cerr << "OnOpenMediaStream failed!" << std::endl;
        return false;
    }

    PCaselessString prefix = connection.GetEndPoint().GetPrefixName();
    std::cerr << (stream.IsSink() ? 
        "streaming media to " : "recording media from ") 
        << prefix << std::endl;

    return true;
}

void RTPUserData::OnTxStatistics(const RTP_Session &session)
{
  std::cerr << __func__ << endl;
}


OpalConnection::AnswerCallResponse Manager::OnAnswerCall(
        OpalConnection &connection,
        const PString &caller)
{
    std::cerr << "Incoming call from " << caller << std::endl;
    std::string val = connection.GetCall().GetToken();
    currentCallToken = val; 
    return OpalConnection::AnswerCallNow;
}

void Manager::OnClosedMediaStream (const OpalMediaStream &stream)
{
    std::cerr << __func__ << std::endl;
}

void Manager::AdjustMediaFormats(
  bool local,                         ///<  Media formats a local ones to be presented to remote
  const OpalConnection & connection,  ///<  Connection that is about to use formats
  OpalMediaFormatList & mediaFormats  ///<  Media formats to use
) const
{
     PStringArray mask("!"+mediaFilter);
     mediaFormats.Remove(mask);
}
 
bool Manager::OnIncomingConnection(OpalConnection &connection, unsigned opts,
        OpalConnection::StringOptions *stropts)
{
    std::cerr << __func__ << ": token=" << connection.GetToken() << std::endl;

    TPState::Instance().SetState(TPState::ESTABLISHED);
    //localep->AcceptIncomingCall(connection.GetCall().GetToken());
    return OpalManager::OnIncomingConnection(connection, opts, stropts);
}

void Manager::OnEstablished(OpalConnection &connection)
{
    std::cerr << __func__ << std::endl;
    TPState::Instance().SetState(TPState::ESTABLISHED);
    OpalManager::OnEstablished(connection);
}


void Manager::OnEstablishedCall(OpalCall &call)
{
    std::cerr << __func__ << std::endl;

    TPState::Instance().SetState(TPState::ESTABLISHED);
    currentCallToken = std::string(
            static_cast<const char*>(call.GetToken()));

    std::cerr << "In call with " << call.GetPartyB() << " using "
        << call.GetPartyA() << " token=[" << currentCallToken << "]" 
        << std::endl;
    OpalManager::OnEstablishedCall(call);
}
                
void Manager::OnReleased(OpalConnection &connection)
{
    OpalConnection::CallEndReason r = connection.GetCallEndReason();
    std::cerr << __func__ <<": reason: " << 
        get_call_end_reason_string(r) << std::endl;

    TPState::Instance().SetState(TPState::CLOSED);
    OpalManager::OnReleased(connection);
}


void Manager::OnClearedCall(OpalCall &call)
{
    std::cerr << __func__ << std::endl;
}

void Manager::OnUserInputTone(OpalConnection &connection,char tone ,int duration)
{
    std::cerr << __func__ << std::endl;
    std::cout << "receive DTMF: [" << tone << "] Duration: " << duration << std::endl;
    OpalManager::OnUserInputTone(connection , tone, duration);
}
