///////////////////////////////////////////////////////////////////////////////
//
//  Module: MailMsg.cpp
//
//    Desc: See MailMsg.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MailMsg.h"

CMailMsg::CMailMsg()
{
   m_sSubject        = _T("");
   m_sMessage        = _T("");
   m_lpCmcLogon      = NULL;
   m_lpCmcSend       = NULL;
   m_lpCmcLogoff     = NULL;
   m_lpMapiLogon     = NULL;
   m_lpMapiSendMail  = NULL;
   m_lpMapiLogoff    = NULL;
   m_lpMapiResolveName = NULL;
   m_lpMapiFreeBuffer = NULL;
   m_bReady          = FALSE;
}

CMailMsg::~CMailMsg()
{
   if (m_bReady)
      Uninitialize();
}

CMailMsg& CMailMsg::SetFrom(CString sAddress, CString sName)
{
   if (m_bReady || Initialize())
   {
      // only one sender allowed
      if (m_from.size())
         m_from.empty();

	  m_from.push_back(TStrStrPair(sAddress,sName));
   }

   return *this;
}

CMailMsg& CMailMsg::SetTo(CString sAddress, CString sName)
{
   if (m_bReady || Initialize())
   {
      // only one recipient allowed
      if (m_to.size())
         m_to.empty();

	  m_to.push_back(TStrStrPair(sAddress,sName));
   }

   return *this;
}

CMailMsg& CMailMsg::SetCc(CString sAddress, CString sName)
{
   if (m_bReady || Initialize())
   {
      m_cc.push_back(TStrStrPair(sAddress,sName));
   }

   return *this;
}

CMailMsg& CMailMsg::SetBc(CString sAddress, CString sName)
{
   if (m_bReady || Initialize())
   {
	   m_bcc.push_back(TStrStrPair(sAddress, sName));
   }

   return *this;
}

CMailMsg& CMailMsg::AddAttachment(CString sAttachment, CString sTitle)
{
   if (m_bReady || Initialize())
   {
      m_attachments.push_back(TStrStrPair(sAttachment, sTitle));
   }

   return *this;
}

BOOL CMailMsg::Send()
{
   // try mapi
   int status = MAPISend();
   if (status != 0)
      return status == 1;
   // try cmc
//   if (CMCSend())
//      return TRUE;

   return FALSE;
}

/*
+------------------------------------------------------------------------------
|
|	Function:	cResolveName()
|
|	Parameters:	[IN]	lpszName = Name of e-mail recipient to resolve.
|				[OUT]	ppRecips = Pointer to a pointer to an lpMapiRecipDesc
|
|	Purpose:	Resolves an e-mail address and returns a pointer to a 
|				MapiRecipDesc structure filled with the recipient information
|				contained in the address book.
|
|	Note:		ppRecips is allocated off the heap using MAPIAllocateBuffer.
|				Any user of this method must be sure to release ppRecips when 
|				done with it using either MAPIFreeBuffer or cFreeBuffer.
+-------------------------------------------------------------------------------
*/
int CMailMsg::cResolveName( LHANDLE m_lhSession, const char * lpszName, lpMapiRecipDesc *ppRecip )
{	
	HRESULT hRes = E_FAIL;
	FLAGS flFlags = 0L;
	ULONG ulReserved = 0L;
	lpMapiRecipDesc pRecips = NULL;
	
	// Always check to make sure there is an active session
	if ( m_lhSession )		
	{
		hRes = m_lpMapiResolveName (
								     m_lhSession,	// Session handle
									 0L,			// Parent window.
									 const_cast<LPSTR>(lpszName),		// Name of recipient.  Passed in by argv.
									 flFlags,		// Flags set to 0 for MAPIResolveName.
									 ulReserved,
									 &pRecips
								  );				

		if ( hRes == SUCCESS_SUCCESS )
		{  
			// Copy the recipient descriptor returned from MAPIResolveName to 
			// the out parameter for this function,
			*ppRecip = pRecips;
		}  
	}
	return hRes;
}



int CMailMsg::MAPISend()
{

   TStrStrVector::iterator p;
   int                  nIndex = 0;
   int                  nRecipients = 0;
   MapiRecipDesc*       pRecipients = NULL;
   MapiRecipDesc*       pOriginator = NULL;
   MapiRecipDesc*       pFirstRecipient = NULL;
   int                  nAttachments = 0;
   MapiFileDesc*        pAttachments = NULL;
   ULONG                status = 0;
   MapiMessage          message;
   std::vector<MapiRecipDesc*>	buffersToFree;
   MapiRecipDesc*       pRecip;

   if (m_bReady || Initialize())
   {
	  LHANDLE hMapiSession;
	  status = m_lpMapiLogon(NULL, NULL, NULL, MAPI_NEW_SESSION | MAPI_LOGON_UI, 0, &hMapiSession);
	  if (SUCCESS_SUCCESS != status) {
		  return FALSE;
	  }

      nRecipients = m_to.size() + m_cc.size() + m_bcc.size() + m_from.size();
      if (nRecipients)
	  {
         pRecipients = new MapiRecipDesc[nRecipients];
		 memset(pRecipients, 0, nRecipients * sizeof  MapiRecipDesc);
	  }

      nAttachments = m_attachments.size();
      if (nAttachments)
         pAttachments = new MapiFileDesc[nAttachments];

      if (pRecipients)
      {
         pFirstRecipient = pRecipients;
         if (m_from.size())
         {
            // set from
			 if (cResolveName(hMapiSession, m_from.begin()->first, &pOriginator) == SUCCESS_SUCCESS) {
				buffersToFree.push_back(pOriginator);
			 }
         }
         if (m_to.size())
         {
			 if (cResolveName(hMapiSession, m_to.begin()->first, &pRecip) == SUCCESS_SUCCESS) {
				if (pFirstRecipient == NULL)
					pFirstRecipient = &pRecipients[nIndex];
				memcpy(&pRecipients[nIndex], pRecip, sizeof pRecipients[nIndex]);
				buffersToFree.push_back(pRecip);
				nIndex++;
			 }
			 else {
				 //MessageBox(NULL,"noo","moo",MB_OK);
				 pRecipients[nIndex].lpEntryID = NULL;
				 pRecipients[nIndex].lpszAddress = m_to.begin()->first.GetBuffer(100);
				 pRecipients[nIndex].lpszName = NULL;
				 pRecipients[nIndex].ulEIDSize = 0;
				 pRecipients[nIndex].ulRecipClass = 1;
				 pRecipients[nIndex].ulReserved = 0;
				 nIndex++;
			 }
         }		
         if (m_cc.size())
         {
            // set cc's
            for (p = m_cc.begin(); p != m_cc.end(); p++, nIndex++)
            {
				if ( cResolveName(hMapiSession, p->first, &pRecip) == SUCCESS_SUCCESS) {
					if (pFirstRecipient == NULL)
						pFirstRecipient = &pRecipients[nIndex];
					memcpy(&pRecipients[nIndex], pRecip, sizeof pRecipients[nIndex]);
					buffersToFree.push_back(pRecip);
					nIndex++;
				}
            }
         }
   
         if (m_bcc.size())
         {
            // set bcc
            for (p = m_bcc.begin(); p != m_bcc.end(); p++, nIndex++)
            {
				if ( cResolveName(hMapiSession, p->first, &pRecip) == SUCCESS_SUCCESS) {
					if (pFirstRecipient == NULL)
						pFirstRecipient = &pRecipients[nIndex];
					memcpy(&pRecipients[nIndex], pRecip, sizeof pRecipients[nIndex]);
					buffersToFree.push_back(pRecip);
					nIndex++;
				}
            }
         }
      }
      if (pAttachments)
      {
         // add attachments
         for (p = m_attachments.begin(), nIndex = 0;
              p != m_attachments.end(); p++, nIndex++)
         {
            pAttachments[nIndex].ulReserved        = 0;
            pAttachments[nIndex].flFlags           = 0;
            pAttachments[nIndex].nPosition         = 0;
            pAttachments[nIndex].lpszPathName      = (LPTSTR)(LPCTSTR)p->first;
            pAttachments[nIndex].lpszFileName      = (LPTSTR)(LPCTSTR)p->second;
            pAttachments[nIndex].lpFileType        = NULL;
         }
      }
	  memset(&message, 0, sizeof message);
      message.ulReserved                        = 0;
	  if (!m_sSubject.IsEmpty())
	      message.lpszSubject                       = (LPTSTR)(LPCTSTR)m_sSubject;
	  else
		  message.lpszSubject = "No Subject";
	  if (!m_sMessage.IsEmpty())
	      message.lpszNoteText                      = (LPTSTR)(LPCTSTR)m_sMessage;
	  else
		  message.lpszNoteText = "No Message Body";
      message.lpszMessageType                   = NULL;
      message.lpszDateReceived                  = NULL;
      message.lpszConversationID                = NULL;
      message.flFlags                           = 0;
      message.lpOriginator                      = pOriginator;
      message.nRecipCount                       = nIndex;
      message.lpRecips                          = pFirstRecipient;
      message.nFileCount                        = nAttachments;
      message.lpFiles                           = pAttachments;

      status = m_lpMapiSendMail(hMapiSession, 0, &message, MAPI_DIALOG, 0);
		  
      m_lpMapiLogoff(hMapiSession, NULL, 0, 0);
	  std::vector<MapiRecipDesc*>::iterator iter;
	  for (iter = buffersToFree.begin(); iter != buffersToFree.end(); iter++) {
		  m_lpMapiFreeBuffer(*iter);
	  }
if (SUCCESS_SUCCESS != status) {
				CString txt;
			txt.Format( "Message did not get sent due to error code %d.\r\n", status ); 
			switch (status)
			{  
			case MAPI_E_AMBIGUOUS_RECIPIENT:
				txt += "A recipient matched more than one of the recipient descriptor structures and MAPI_DIALOG was not set. No message was sent.\r\n" ;
				break;
			case MAPI_E_ATTACHMENT_NOT_FOUND:
				txt += "The specified attachment was not found. No message was sent.\r\n" ;
				break;
			case MAPI_E_ATTACHMENT_OPEN_FAILURE:
				txt += "The specified attachment could not be opened. No message was sent.\r\n" ;
				break;
			case MAPI_E_BAD_RECIPTYPE:
				txt += "The type of a recipient was not MAPI_TO, MAPI_CC, or MAPI_BCC. No message was sent.\r\n" ;
				break;
			case MAPI_E_FAILURE:
				txt += "One or more unspecified errors occurred. No message was sent.\r\n" ;
				break;
			case MAPI_E_INSUFFICIENT_MEMORY:
				txt += "There was insufficient memory to proceed. No message was sent.\r\n" ;
				break;
			case MAPI_E_INVALID_RECIPS:
				txt += "One or more recipients were invalid or did not resolve to any address.\r\n" ;
				break;
			case MAPI_E_LOGIN_FAILURE:
				txt += "There was no default logon, and the user failed to log on successfully when the logon dialog box was displayed. No message was sent.\r\n" ;
				break;
			case MAPI_E_TEXT_TOO_LARGE:
				txt += "The text in the message was too large. No message was sent.\r\n" ;
				break;
			case MAPI_E_TOO_MANY_FILES:
				txt += "There were too many file attachments. No message was sent.\r\n" ;
				break;
			case MAPI_E_TOO_MANY_RECIPIENTS:
				txt += "There were too many recipients. No message was sent.\r\n" ;
				break;
			case MAPI_E_UNKNOWN_RECIPIENT:
				txt += "A recipient did not appear in the address list. No message was sent.\r\n" ;
				break;
			case MAPI_E_USER_ABORT:
				txt += "The user canceled one of the dialog boxes. No message was sent.\r\n" ;
				break;
			default:
				txt += "Unknown error code.\r\n" ;
				break;
			}
			::MessageBox(0, txt, "Error", MB_OK);
}

      if (pRecipients)
         delete [] pRecipients;

      if (nAttachments)
         delete [] pAttachments;
   }

   if (SUCCESS_SUCCESS == status)
	   return 1;
   if (MAPI_E_USER_ABORT == status)
	   return -1;
   // other failure
   return 0;
}

BOOL CMailMsg::CMCSend()
{
   TStrStrVector::iterator p;
   int                  nIndex = 0;
   CMC_recipient*       pRecipients;
   CMC_attachment*      pAttachments;
   CMC_session_id       session;
   CMC_return_code      status = 0;
   CMC_message          message;
   CMC_boolean          bAvailable = FALSE;
   CMC_time             t_now = {0};

   if (m_bReady || Initialize())
   {
      pRecipients = new CMC_recipient[m_to.size() + m_cc.size() + m_bcc.size() + m_from.size()];
      pAttachments = new CMC_attachment[m_attachments.size()];

      // set cc's
      for (p = m_cc.begin(); p != m_cc.end(); p++, nIndex++)
      {
         pRecipients[nIndex].name                = (LPTSTR)(LPCTSTR)p->second;
         pRecipients[nIndex].name_type           = CMC_TYPE_INDIVIDUAL;
         pRecipients[nIndex].address             = (LPTSTR)(LPCTSTR)p->first;
         pRecipients[nIndex].role                = CMC_ROLE_CC;
         pRecipients[nIndex].recip_flags         = 0;
         pRecipients[nIndex].recip_extensions    = NULL;
      }
   
      // set bcc
      for (p = m_bcc.begin(); p != m_bcc.end(); p++, nIndex++)
      {
         pRecipients[nIndex].name                = (LPTSTR)(LPCTSTR)p->second;
         pRecipients[nIndex].name_type           = CMC_TYPE_INDIVIDUAL;
         pRecipients[nIndex].address             = (LPTSTR)(LPCTSTR)p->first;
         pRecipients[nIndex].role                = CMC_ROLE_BCC;
         pRecipients[nIndex].recip_flags         = 0;
         pRecipients[nIndex].recip_extensions    = NULL;
      }
   
      // set to
      pRecipients[nIndex].name                   = (LPTSTR)(LPCTSTR)m_to.begin()->second;
      pRecipients[nIndex].name_type              = CMC_TYPE_INDIVIDUAL;
      pRecipients[nIndex].address                = (LPTSTR)(LPCTSTR)m_to.begin()->first;
      pRecipients[nIndex].role                   = CMC_ROLE_TO;
      pRecipients[nIndex].recip_flags            = 0;
      pRecipients[nIndex].recip_extensions       = NULL;
   
      // set from
      pRecipients[nIndex+1].name                 = (LPTSTR)(LPCTSTR)m_from.begin()->second;
      pRecipients[nIndex+1].name_type            = CMC_TYPE_INDIVIDUAL;
      pRecipients[nIndex+1].address              = (LPTSTR)(LPCTSTR)m_from.begin()->first;
      pRecipients[nIndex+1].role                 = CMC_ROLE_ORIGINATOR;
      pRecipients[nIndex+1].recip_flags          = CMC_RECIP_LAST_ELEMENT;
      pRecipients[nIndex+1].recip_extensions     = NULL;
   
      // add attachments
      for (p = m_attachments.begin(), nIndex = 0;
           p != m_attachments.end(); p++, nIndex++)
      {
         pAttachments[nIndex].attach_title       = (LPTSTR)(LPCTSTR)p->second;
         pAttachments[nIndex].attach_type        = NULL;
         pAttachments[nIndex].attach_filename    = (LPTSTR)(LPCTSTR)p->first;
         pAttachments[nIndex].attach_flags       = 0;
         pAttachments[nIndex].attach_extensions  = NULL;
      }
      pAttachments[nIndex-1].attach_flags        = CMC_ATT_LAST_ELEMENT;

      message.message_reference                 = NULL;
      message.message_type                      = NULL;
	  if (m_sSubject.IsEmpty())
		  message.subject = "No Subject";
	  else
	      message.subject                           = (LPTSTR)(LPCTSTR)m_sSubject;
      message.time_sent                         = t_now;
	  if (m_sMessage.IsEmpty())
		  message.text_note = "No Body";
	  else
		  message.text_note                         = (LPTSTR)(LPCTSTR)m_sMessage;
      message.recipients                        = pRecipients;
      message.attachments                       = pAttachments;
      message.message_flags                     = 0;
      message.message_extensions                = NULL;

      status = m_lpCmcQueryConfiguration(
                  0, 
                  CMC_CONFIG_UI_AVAIL, 
                  (void*)&bAvailable, 
                  NULL
                  );

      if (CMC_SUCCESS == status && bAvailable)
      {
         status = m_lpCmcLogon(
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     0,
                     CMC_VERSION,
                     CMC_LOGON_UI_ALLOWED |
                     CMC_ERROR_UI_ALLOWED,
                     &session,
                     NULL
                     );

         if (CMC_SUCCESS == status)
         {
            status = m_lpCmcSend(session, &message, 0, 0, NULL);

            m_lpCmcLogoff(session, NULL, CMC_LOGON_UI_ALLOWED, NULL);
         }
      }

      delete [] pRecipients;
      delete [] pAttachments;
   }

   return ((CMC_SUCCESS == status) && bAvailable);
}

BOOL CMailMsg::Initialize()
{
   m_hMapi = ::LoadLibrary(_T("mapi32.dll"));
   
   if (!m_hMapi)
      return FALSE;

   m_lpCmcQueryConfiguration = (LPCMCQUERY)::GetProcAddress(m_hMapi, _T("cmc_query_configuration"));
   m_lpCmcLogon = (LPCMCLOGON)::GetProcAddress(m_hMapi, _T("cmc_logon"));
   m_lpCmcSend = (LPCMCSEND)::GetProcAddress(m_hMapi, _T("cmc_send"));
   m_lpCmcLogoff = (LPCMCLOGOFF)::GetProcAddress(m_hMapi, _T("cmc_logoff"));
   m_lpMapiLogon = (LPMAPILOGON)::GetProcAddress(m_hMapi, _T("MAPILogon"));
   m_lpMapiSendMail = (LPMAPISENDMAIL)::GetProcAddress(m_hMapi, _T("MAPISendMail"));
   m_lpMapiLogoff = (LPMAPILOGOFF)::GetProcAddress(m_hMapi, _T("MAPILogoff"));
   m_lpMapiResolveName = (LPMAPIRESOLVENAME) GetProcAddress(m_hMapi, _T("MAPIResolveName"));
   m_lpMapiFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress(m_hMapi, _T("MAPIFreeBuffer"));

   m_bReady = (m_lpCmcLogon && m_lpCmcSend && m_lpCmcLogoff) ||
              (m_lpMapiLogon && m_lpMapiSendMail && m_lpMapiLogoff);

   return m_bReady;
}

void CMailMsg::Uninitialize()
{
   ::FreeLibrary(m_hMapi);
}