#pragma once

#define AT_VERSION_MAJOR         2024
#define AT_VERSION_MINOR         4
#define AT_VERSION_PATCH         0
#define AT_VERSION_BUILD         2

#define ATSTR_(x)                #x
#define ATSTR(x)                 ATSTR_(x)

#define AT_PRODUCT_PAGE 	      L"https://alttab.sourceforge.io/"
#define AT_PRODUCT_LATEST_URL    L"https://sourceforge.net/projects/alttab/files/latest/download"
#define AT_UPDATE_FILE_URL       L"https://sourceforge.net/projects/alttab/files/version.txt/download"
#define AT_AUTHOR_NAME 		      L"Lokesh Govindu"
#define AT_AUTHOR_PAGE 		      L"https://github.com/lokeshgovindu"
#define AT_AUTHOR_EMAIL          L"lokeshgovindu@gmail.com"

#define AT_VERSION_VSINFO        AT_VERSION_MAJOR,AT_VERSION_MINOR,AT_VERSION_PATCH,AT_VERSION_BUILD
#define AT_VERSION_TEXT          ATSTR(AT_VERSION_MAJOR.AT_VERSION_MINOR.AT_VERSION_PATCH.AT_VERSION_BUILD)
#define AT_VERSION_TEXTW         TEXT(ATSTR(AT_VERSION_MAJOR.AT_VERSION_MINOR.AT_VERSION_PATCH.AT_VERSION_BUILD))
#define AT_FULL_VERSIONA         AT_VERSION_TEXT
#define AT_FULL_VERSIONW         AT_VERSION_TEXTW
#define AT_SHORT_VERSIONA        ATSTR(AT_VERSION_MAJOR.AT_VERSION_MINOR)
#define AT_SHORT_VERSIONW        TEXT(AT_SHORT_VERSIONA)
#define AT_PRODUCT_YEARA         ATSTR(AT_VERSION_MAJOR)
#define AT_PRODUCT_YEARW         TEXT(AT_PRODUCT_YEARA)
#define AT_PRODUCT_NAME          "AltTab"
#define AT_PRODUCT_NAMEA         AT_PRODUCT_NAME
#define AT_PRODUCT_NAMEW         TEXT(AT_PRODUCT_NAME)
#define AT_PRODUCT_VERSIONA      ATSTR(AT_PRODUCT_YEARA.AT_VERSION_MINOR)
#define AT_PRODUCT_VERSIONW      ATSTR(AT_PRODUCT_YEARA.AT_VERSION_MINOR)
#define AT_LEGAL_COPYRIGHT       "Copyright © 2024 Lokesh Govindu"
#define AT_FILE_DESCRIPTION      "AltTab Switcher"
