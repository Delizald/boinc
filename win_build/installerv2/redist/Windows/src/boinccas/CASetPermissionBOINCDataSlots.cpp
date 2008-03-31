// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "stdafx.h"
#include "boinccas.h"
#include "CASetPermissionBOINCDataSlots.h"
#include "dirops.h"

#define CUSTOMACTION_NAME               _T("CASetPermissionBOINCDataSlots")
#define CUSTOMACTION_PROGRESSTITLE      _T("Setting permissions on the BOINC Slots directory.")


/////////////////////////////////////////////////////////////////////
// 
// Function:    
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
CASetPermissionBOINCDataSlots::CASetPermissionBOINCDataSlots(MSIHANDLE hMSIHandle) :
    BOINCCABase(hMSIHandle, CUSTOMACTION_NAME, CUSTOMACTION_PROGRESSTITLE)
{}


/////////////////////////////////////////////////////////////////////
// 
// Function:    
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
CASetPermissionBOINCDataSlots::~CASetPermissionBOINCDataSlots()
{
    BOINCCABase::~BOINCCABase();
}


/////////////////////////////////////////////////////////////////////
// 
// Function:    
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
UINT CASetPermissionBOINCDataSlots::OnExecution()
{
    DWORD               dwRes = 0;
    PACL                pACL = NULL;
    PSID                psidAdministrators = NULL;
    EXPLICIT_ACCESS     ea[4];
    tstring             strBOINCAdminsGroupAlias;
    tstring             strBOINCUsersGroupAlias;
    tstring             strBOINCProjectsGroupAlias;
    tstring             strBOINCDataDirectory;
    tstring             strBOINCDataSlotsDirectory;
    UINT                uiReturnValue = -1;

    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

    uiReturnValue = GetProperty( _T("BOINC_ADMINS_GROUPNAME"), strBOINCAdminsGroupAlias );
    if ( uiReturnValue ) return uiReturnValue;

    uiReturnValue = GetProperty( _T("BOINC_USERS_GROUPNAME"), strBOINCUsersGroupAlias );
    if ( uiReturnValue ) return uiReturnValue;

    uiReturnValue = GetProperty( _T("BOINC_PROJECTS_GROUPNAME"), strBOINCProjectsGroupAlias );
    if ( uiReturnValue ) return uiReturnValue;

    uiReturnValue = GetProperty( _T("DATADIR"), strBOINCDataDirectory );
    if ( uiReturnValue ) return uiReturnValue;


    strBOINCDataSlotsDirectory = strBOINCDataDirectory + _T("\\slots");


    // Initialize an EXPLICIT_ACCESS structure for all ACEs.
    ZeroMemory(&ea, 4 * sizeof(EXPLICIT_ACCESS));

    // Administrators
    AllocateAndInitializeSid(
        &SIDAuthNT,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &psidAdministrators
    );

    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR)psidAdministrators;

    // boinc_admins
    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR)strBOINCAdminsGroupAlias.c_str();

    // boinc_users
    ea[2].grfAccessPermissions = GENERIC_READ;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[2].Trustee.ptstrName  = (LPTSTR)strBOINCUsersGroupAlias.c_str();

    // boinc_projects
    ea[3].grfAccessPermissions = GENERIC_ALL;
    ea[3].grfAccessMode = SET_ACCESS;
    ea[3].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[3].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[3].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[3].Trustee.ptstrName  = (LPTSTR)strBOINCProjectsGroupAlias.c_str();


    // Create a new ACL that contains the new ACEs.
    dwRes = SetEntriesInAcl(4, &ea[0], NULL, &pACL);
    if (ERROR_SUCCESS != dwRes) 
    {
        LogMessage(
            INSTALLMESSAGE_ERROR,
            NULL, 
            NULL,
            NULL,
            GetLastError(),
            _T("SetEntriesInAcl Error")
        );
        return ERROR_INSTALL_FAILURE;
    }

    // Set the ACL on the Data Directory itself.
    dwRes = SetNamedSecurityInfo( 
        (LPWSTR)strBOINCDataSlotsDirectory.c_str(),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        pACL,
        NULL
    );
    if (ERROR_SUCCESS != dwRes) 
    {
        LogMessage(
            INSTALLMESSAGE_ERROR,
            NULL, 
            NULL,
            NULL,
            GetLastError(),
            _T("SetNamedSecurityInfo Error")
        );
        return ERROR_INSTALL_FAILURE;
    }

    // Set ACLs on all files and sub folders.
    RecursiveSetPermissions(strBOINCDataSlotsDirectory, pACL);


    if (pACL) 
        LocalFree(pACL);
    if (psidAdministrators)
        LocalFree(psidAdministrators);

    return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
// 
// Function:    SetPermissionBOINCDataSlots
//
// Description: 
//
/////////////////////////////////////////////////////////////////////
UINT __stdcall SetPermissionBOINCDataSlots(MSIHANDLE hInstall)
{
    UINT uiReturnValue = 0;

    CASetPermissionBOINCDataSlots* pCA = new CASetPermissionBOINCDataSlots(hInstall);
    uiReturnValue = pCA->Execute();
    delete pCA;

    return uiReturnValue;
}

