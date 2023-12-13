#pragma once

#define AT_VERSION_MAJOR      2023
#define AT_VERSION_MINOR      12
#define AT_VERSION_PATCH      0
#define AT_VERSION_BUILD      2

#define ATSTR_(x)             #x
#define ATSTR(x)              ATSTR_(x)

#define AT_VERSION_VSINFO     AT_VERSION_MAJOR,AT_VERSION_MINOR,AT_VERSION_PATCH,AT_VERSION_BUILD
#define AT_VERSION_TEXT       ATSTR(AT_VERSION_MAJOR.AT_VERSION_MINOR.AT_VERSION_PATCH.AT_VERSION_BUILD)
#define AT_PRODUCT_NAME       "AltTab"
#define AT_PRODUCT_NAMEA      AT_PRODUCT_NAME
#define AT_PRODUCT_NAMEW      TEXT(AT_PRODUCT_NAME)
#define AT_LEGAL_COPYRIGHT    "Copyright (C) 2023 Lokesh Govindu"
#define AT_FILE_DESCRIPTION   "AltTab Switcher"
