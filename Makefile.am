# -*- Makefile -*-

ACLOCAL_AMFLAGS = -I autoconf

SUBDIRS = .
if ENABLE_BASE
SUBDIRS += ECtools/archiver/include/kopano \
	common/include/kopano m4lcommon/include/kopano \
	mapi4linux/include inetmapi/include/inetmapi provider/include \
	installer installer/quotamail installer/userscripts po
if ENABLE_PYBIND
SUBDIRS += swig swig/python
endif
endif # ENABLE_BASE
SUBDIRS += installer/linux
if ENABLE_PYTHON
SUBDIRS += ECtools/backup \
	ECtools/search ECtools/spamd \
	ECtools/utils installer/searchscripts spooler/python \
	spooler/python/plugins swig/python/kopano
endif
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST = .astylerc .clang-format .clang_complete .editorconfig .gitignore \
	CONTRIBUTING.md README.md cppcheck.suppressions \
	doc/Code_structure.md doc/Database_structure.md doc/adminssl.txt \
	doc/classes.gv doc/extract_user.txt doc/hl.txt doc/install.txt \
	doc/kopano-autorespond.py.8 doc/sanitizer.txt doc/sql_attach.txt \
	doc/sql_messages.txt doc/sql_searchfolders.txt doc/sql_spooler.txt \
	provider/client/README.md provider/libserver/README.md \
	tools/describe_version \
	tests/mails/big5.eml tests/mails/encoded-word-split.eml \
	tests/mails/gb2312_18030.eml tests/mails/html-charset-01.eml \
	tests/mails/html-charset-02.eml tests/mails/iconvonly01.eml \
	tests/mails/iconvonly02.eml tests/mails/kc-138-1.eml \
	tests/mails/kc-138-2.eml tests/mails/mime_charset_01.eml \
	tests/mails/mime_charset_02.eml tests/mails/mime_charset_03.eml \
	tests/mails/no-charset-01.eml tests/mails/no-charset-02.eml \
	tests/mails/no-charset-03.eml tests/mails/no-charset-07.eml \
	tests/mails/no-content-type-alt.eml tests/mails/no-content-type.eml \
	tests/mails/unknown-text-charset.eml \
	tests/mails/unknown-transfer-enc.eml tests/mails/zcp-11581.eml \
	tests/mails/zcp-11699-p.eml tests/mails/zcp-11699-ub.eml \
	tests/mails/zcp-11699-utf8.eml tests/mails/zcp-11713.eml \
	tests/mails/zcp-12930.eml tests/mails/zcp-13036-0db504a2.eml \
	tests/mails/zcp-13036-6906a338.eml tests/mails/zcp-13036-lh.eml \
	tests/mails/zcp-13175.eml tests/mails/zcp-13337.eml \
	tests/mails/zcp-13439-nl.eml tests/mails/zcp-13449-meca.eml \
	tests/mails/zcp-13449-na.eml tests/mails/zcp-13473.eml


AM_CPPFLAGS = ${ZCPPFLAGS} -DLDAP_THREAD_SAFE \
	-DMAPICONFIGDIR=\"${prefix}/lib/mapi.d:${sysconfdir}/mapi.d\" -DPKGLIBDIR='"${pkglibdir}"' \
	-DUSCRIPTDIR='"${uscriptlibdir}"' -DUSCRIPTLDIR='"${uscriptconfdir}"' \
	-DSOAP_CMAC='__attribute__((visibility("default")))' \
	-DSOAP_FMAC3=SOAP_CMAC -DSOAP_FMAC5=SOAP_CMAC -DSOAP_NMAC=SOAP_CMAC \
	-Iprovider/soap \
	-I${top_srcdir}/common \
	-I${top_srcdir}/libfreebusy \
	-I${top_srcdir}/m4lcommon \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/provider/client \
	-I${top_srcdir}/provider/common \
	-I${top_srcdir}/provider/include \
	-I${top_srcdir}/provider/libserver \
	-I${top_srcdir}/provider/plugins \
	-I${top_srcdir}/libfreebusy \
	-I${top_srcdir}/libicalmapi \
	-I${top_srcdir}/ECtools/archiver \
	${CRYPTO_CFLAGS} ${curl_CFLAGS} ${dbcxx_CPPFLAGS} \
	${GSOAP_CFLAGS} ${ICAL_CFLAGS} ${idn_CFLAGS} ${jsoncpp_CFLAGS} \
	${KRB5_CFLAGS} ${LDAP_FLAGS} ${libHX_CFLAGS} \
	${kcoidc_CFLAGS} ${kustomer_CFLAGS} \
	${MYSQL_INCLUDES} ${SSL_CFLAGS} \
	${s3_CFLAGS} ${kcoidc_CFLAGS} ${TCMALLOC_CFLAGS} \
	${VMIME_CFLAGS} ${xapian_CFLAGS} ${XML2_CFLAGS}
AM_CXXFLAGS = ${ZCXXFLAGS} -Wno-sign-compare


#
# target programs
#
dist_bin_SCRIPTS =
if ENABLE_BASE
bin_PROGRAMS = \
	kopano-fsck kopano-ibrule kopano-oof kopano-passwd kopano-stats \
	kopano-vcfimport
dist_bin_SCRIPTS += ECtools/migration-imap/kopano-migration-imap
sbin_PROGRAMS = \
	kopano-admin kopano-archiver kopano-dagent kopano-dbadm kopano-gateway \
	kopano-ical kopano-monitor kopano-server kopano-spooler \
	kopano-srvadm kopano-storeadm
noinst_PROGRAMS = kopano-cfgchecker
pkglibexec_PROGRAMS = eidprint kscriptrun mapitime setupenv
setupenv_SOURCES = tests/setupenv.cpp
setupenv_LDADD = libkcutil.la
check_PROGRAMS = tests/ablookup tests/htmltext tests/imtomapi \
	tests/kc-335 tests/kc-1759 tests/mapialloctime \
	tests/readflag tests/ustring tests/zcpmd5 tests/chtmltotextparsertest \
	tests/rtfhtmltest
if HAVE_CPPUNIT
check_PROGRAMS += tests/mapisuite
endif
noinst_PROGRAMS += ${check_PROGRAMS}
endif # ENABLE_BASE

TESTS = tests/chtmltotextparsertest tests/rtfhtmltest

if ENABLE_PYTHON
dist_sbin_SCRIPTS = ECtools/utils/kopano-mailbox-permissions \
	ECtools/archiver/scripts/kopano-archiver-aclset \
	ECtools/archiver/scripts/kopano-archiver-aclsync \
	ECtools/archiver/scripts/kopano-archiver-restore \
	tools/python-scripts/kopano-fix-ipm-subtree \
	tools/python-scripts/kopano-recreate-systemfolders \
	tools/python-scripts/kopano-cachestat
endif # ENABLE_PYTHON


if ENABLE_BASE

#
# target libraries
#
# `make install` ignores both *_LIBADD, but also calls libtool for relinking,
# so the order of lib_LTLIBRARIES is important! Dependant libs go to the right.
# (Order was previously implied by the SUBDIRS variable.)
#
lib_LTLIBRARIES = \
	libkcutil.la libmapi.la libkcfreebusy.la \
	libkcsoap.la libkcserver.la \
	libkcicalmapi.la libkcinetmapi.la libkcarchiver.la
noinst_LTLIBRARIES =
noinst_DATA = \
	kopano-admin.ldx \
	kopano-archiver.ldx kopano-cfgchecker.ldx kopano-dagent.ldx \
	kopano-dbadm.ldx kopano-fsck.ldx \
	kopano-gateway.ldx kopano-ibrule.ldx kopano-ical.ldx \
	kopano-monitor.ldx kopano-oof.ldx \
	kopano-passwd.ldx kopano-server.ldx kopano-spooler.ldx \
	kopano-srvadm.ldx kopano-storeadm.ldx kopano-stats.ldx \
	libkcarchiver.ldd libkcclient.ldd \
	libkccontacts.ldd libkcfreebusy.ldd libkcicalmapi.ldd \
	libkcinetmapi.ldd \
	libkcserver.ldd libkcutil.ldd libmapi.ldd
if ENABLE_PYBIND
lib_LTLIBRARIES += libkcpyplug.la
noinst_DATA += libkcpyplug.ldd
endif
if HAVE_RRD
pkglibexec_PROGRAMS += kopano-statsd
noinst_DATA += kopano-statsd.ldx
endif
# Undefined references
#noinst_DATA += libkcsoap.ldd
pkglib_LTLIBRARIES = libkcclient.la libkccontacts.la
dovmoddir = ${libdir}/dovecot/modules
if WITH_DOVECOT
dovmod_LTLIBRARIES = libauthdb_mapi.la libdovemapi_plugin.la
endif


#
# headers and other static files
#
pkginclude_HEADERS = \
	provider/common/include/kopano/pcuser.hpp \
	provider/libserver/include/kopano/ECPluginSharedData.h
noinst_HEADERS =
pkgconfdir = ${libdir}/pkgconfig
pkgconf_DATA = common/kopano.pc
mapiinfdir = ${prefix}/lib/mapi.d
dist_mapiinf_DATA = provider/client/kopano.inf provider/contacts/zcontacts.inf
CLEANFILES += ${noinst_DATA}
EXTRA_DIST += php-ext/mapi.sym
default_sym = ${top_builddir}/default.sym
default_SYFLAGS = -Wl,--version-script=${default_sym}
plugin_SYFLAGS = -Wl,--version-script=provider/plugin.sym


#
# kopano-admin
#
kopano_admin_SOURCES = ECtools/admin/admin.cpp
kopano_admin_LDADD = \
	libmapi.la libkcutil.la libkcarchiver.la ${SSL_LIBS}


#
# kopano-archiver
#
kopano_archiver_SOURCES = ECtools/archiver/main.cpp
kopano_archiver_LDADD = libmapi.la libkcutil.la libkcarchiver.la


#
# kopano-cfgchecker
#
kopano_cfgchecker_SOURCES = \
	ECtools/cfgchecker/ECConfigCheck.cpp ECtools/cfgchecker/ECConfigCheck.h \
	ECtools/cfgchecker/LDAPConfigCheck.cpp \
	ECtools/cfgchecker/ServerConfigCheck.cpp \
	ECtools/cfgchecker/cfg.cpp
kopano_cfgchecker_LDADD = libkcutil.la libmapi.la ${ICONV_LIBS}


#
# kopano-dagent
#
kopano_dagent_SOURCES = \
	spooler/DAgent.cpp spooler/LMTP.cpp spooler/LMTP.h \
	spooler/archive.cpp spooler/archive.h \
	spooler/rules.cpp spooler/rules.h
kopano_dagent_LDADD = \
	libkcinetmapi.la libmapi.la libkcutil.la libkcarchiver.la -lpthread \
	${DL_LIBS} ${SSL_LIBS} ${XML2_LIBS} ${icu_uc_LIBS}


#
# kopano-dbadm
#
kopano_dbadm_SOURCES = ECtools/dbadm.cpp
kopano_dbadm_CPPFLAGS = ${AM_CPPFLAGS}
kopano_dbadm_LDADD = libkcutil.la libkcserver.la ${MYSQL_LIBS}


#
# kopano-fsck
#
kopano_fsck_SOURCES = \
	ECtools/fsck/fsck.cpp ECtools/fsck/fsck-main.cpp ECtools/fsck/fsck-calendar.cpp \
	ECtools/fsck/fsck-contact.cpp ECtools/fsck/fsck-task.cpp ECtools/fsck/fsck.h
kopano_fsck_LDADD = libmapi.la libkcutil.la


#
# kopano-gateway
#
kopano_gateway_SOURCES = \
	gateway/ClientProto.h gateway/Gateway.cpp \
	gateway/IMAP.cpp gateway/IMAP.h \
	gateway/POP3.cpp gateway/POP3.h
kopano_gateway_LDADD = \
	libkcinetmapi.la libmapi.la libkcutil.la -lpthread \
	${CRYPTO_LIBS} ${SSL_LIBS} ${XML2_LIBS} ${icu_uc_LIBS}


kopano_ibrule_SOURCES = ECtools/ibrule.cpp
kopano_ibrule_LDADD = libmapi.la libkcutil.la ${libHX_LIBS}


#
# kopano-ical
#
kopano_ical_SOURCES = \
	caldav/CalDAV.cpp caldav/CalDavProto.cpp caldav/CalDavProto.h \
	caldav/CalDavUtil.cpp caldav/CalDavUtil.h \
	caldav/Http.cpp caldav/Http.h \
	caldav/ProtocolBase.cpp caldav/ProtocolBase.h \
	caldav/WebDav.cpp caldav/WebDav.h caldav/iCal.cpp caldav/iCal.h
kopano_ical_LDADD = \
	libmapi.la libkcutil.la libkcicalmapi.la libkcfreebusy.la -lpthread \
	${ICAL_LIBS} ${ICONV_LIBS} ${icu_uc_LIBS} ${SSL_LIBS} ${XML2_LIBS}


#
# kopano-monitor
#
kopano_monitor_SOURCES = \
	ECtools/monitor/ECMonitorDefs.h \
	ECtools/monitor/ECQuotaMonitor.cpp ECtools/monitor/ECQuotaMonitor.h \
	ECtools/monitor/monitor.cpp
kopano_monitor_LDADD = libmapi.la libkcutil.la


kopano_oof_SOURCES = ECtools/oof.cpp
kopano_oof_LDADD = ${jsoncpp_LIBS} ${libHX_LIBS} libmapi.la libkcutil.la


#
# kopano-passwd
#
kopano_passwd_SOURCES = ECtools/passwd/passwd.cpp
kopano_passwd_LDADD = libmapi.la libkcutil.la


#
# kopano-spooler
#
kopano_spooler_SOURCES = \
	spooler/Spooler.cpp spooler/archive.cpp spooler/archive.h \
	spooler/mailer.cpp spooler/mailer.h
kopano_spooler_LDADD = \
	libkcinetmapi.la libmapi.la libkcutil.la libkcarchiver.la \
	-lpthread ${DL_LIBS} ${INTL_LIBS} ${XML2_LIBS}


kopano_srvadm_SOURCES = ECtools/srvadm.cpp
kopano_srvadm_LDADD = ${libHX_LIBS} libmapi.la libkcutil.la


#
# kopano-stats
#
kopano_stats_SOURCES = ECtools/stats/stats.cpp
kopano_stats_CPPFLAGS = ${AM_CPPFLAGS} ${NCURSES_FLAGS}
# ncurses needs to come first, because ICU_LIBS erroneously has -L/usr/lib64
# which overrides the ncurses lib selection >:-(
kopano_stats_LDADD = ${NCURSES_LIBS} libmapi.la libkcutil.la


kopano_storeadm_SOURCES = ECtools/storeadm.cpp
kopano_storeadm_LDADD = ${jsoncpp_LIBS} ${libHX_LIBS} libmapi.la libkcutil.la


#
# kopano-statsd
#
kopano_statsd_SOURCES = ECtools/statsd.cpp provider/soap/emptysoapC.cpp
kopano_statsd_LDADD = libkcutil.la ${GSOAP_LIBS} ${jsoncpp_LIBS} ${libHX_LIBS} ${rrd_LIBS}


#
# kopano-server
#
kopano_server_SOURCES = \
	provider/server/ECServer.cpp \
	provider/server/ECSoapServerConnection.cpp provider/server/ECSoapServerConnection.h \
	provider/server/ECThreadManager.cpp provider/server/ECThreadManager.h
kopano_server_LDADD = \
	libkcutil.la libkcserver.la libkcsoap.la ${GSOAP_LIBS} \
	${clock_LIBS} ${CRYPTO_LIBS} ${DL_LIBS} ${libHX_LIBS} ${icu_uc_LIBS} \
	${icu_i18n_LIBS} \
	${MYSQL_LIBS} ${SSL_LIBS} ${PAM_LIBS} ${KRB5_LIBS} ${kcoidc_LIBS} ${kustomer_LIBS} \
	-lpthread
kopano_server_LDFLAGS = ${AM_LDFLAGS}


kopano_vcfimport_SOURCES = ECtools/vcfimport.cpp
kopano_vcfimport_LDADD = libmapi.la libkcutil.la libkcicalmapi.la ${libHX_LIBS}


#
# libkcarchiver
#
libkcarchiver_la_SOURCES = \
	ECtools/archiver/ArchiverSession.cpp ECtools/archiver/ArchiverSession.h \
	ECtools/archiver/archiver-common.cpp \
	ECtools/archiver/ArchiveManageImpl.cpp ECtools/archiver/ArchiveManageImpl.h \
	ECtools/archiver/ArchiveManage.h \
	ECtools/archiver/ArchiveStateCollector.cpp ECtools/archiver/ArchiveStateCollector.h \
	ECtools/archiver/ArchiveStateUpdater.cpp ECtools/archiver/ArchiveStateUpdater.h \
	ECtools/archiver/helpers/ArchiveHelper.cpp ECtools/archiver/helpers/ArchiveHelper.h \
	ECtools/archiver/helpers/StoreHelper.cpp ECtools/archiver/helpers/StoreHelper.h \
	ECtools/archiver/helpers/MAPIPropHelper.cpp ECtools/archiver/helpers/MAPIPropHelper.h \
	ECtools/archiver/ECArchiverLogger.cpp ECtools/archiver/ECArchiverLogger.h \
	ECtools/archiver/ECIterators.cpp ECtools/archiver/ECIterators.h \
	ECtools/archiver/Archiver.cpp ECtools/archiver/Archiver.h \
	ECtools/archiver/ArchiverImpl.cpp ECtools/archiver/ArchiverImpl.h \
	ECtools/archiver/operations/instanceidmapper.cpp \
	ECtools/archiver/operations/instanceidmapper.h \
	ECtools/archiver/operations/operations.cpp \
	ECtools/archiver/operations/operations.h \
	ECtools/archiver/operations/copier.cpp \
	ECtools/archiver/operations/copier.h \
	ECtools/archiver/operations/deleter.cpp \
	ECtools/archiver/operations/deleter.h \
	ECtools/archiver/operations/stubber.cpp \
	ECtools/archiver/operations/stubber.h \
	ECtools/archiver/operations/transaction.cpp \
	ECtools/archiver/operations/transaction.h \
	ECtools/archiver/operations/postsaveaction.h \
	ECtools/archiver/operations/postsaveiidupdater.cpp \
	ECtools/archiver/operations/postsaveiidupdater.h \
	ECtools/archiver/ArchiveControlImpl.cpp ECtools/archiver/ArchiveControlImpl.h \
	ECtools/archiver/arc_mysql.cpp ECtools/archiver/arc_mysql.hpp \
	common/database.cpp common/include/kopano/database.hpp
libkcarchiver_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined \
	${default_SYFLAGS${NO_VSYM}}
libkcarchiver_la_LIBADD = libmapi.la libkcutil.la \
	${INTL_LIBS} ${MYSQL_LIBS} ${SSL_LIBS}
EXTRA_libkcarchiver_la_DEPENDENCIES = ${default_sym}


#
# libkcclient
#
libkcclient_la_SOURCES = \
	provider/client/ClientUtil.cpp provider/client/ClientUtil.h \
	provider/client/ECABContainer.cpp provider/client/ECABContainer.h \
	provider/client/ECArchiveAwareMessage.cpp provider/client/ECArchiveAwareMessage.h \
	provider/client/ECAttach.cpp provider/client/ECAttach.h \
	provider/client/ECChangeAdvisor.cpp provider/client/ECChangeAdvisor.h \
	provider/client/ECExchangeExportChanges.cpp provider/client/ECExchangeExportChanges.h \
	provider/client/ECExchangeImportContentsChanges.cpp \
	provider/client/ECExchangeImportContentsChanges.h \
	provider/client/ECExchangeImportHierarchyChanges.cpp \
	provider/client/ECExchangeImportHierarchyChanges.h \
	provider/client/ECExchangeModifyTable.cpp provider/client/ECExchangeModifyTable.h \
	provider/client/ECExportAddressbookChanges.cpp \
	provider/client/ECExportAddressbookChanges.h \
	provider/client/ECGenericProp.cpp provider/client/ECGenericProp.h \
	provider/client/ics_client.hpp \
	provider/client/ECMAPIContainer.cpp provider/client/ECMAPIContainer.h \
	provider/client/ECMAPIFolder.cpp provider/client/ECMAPIFolder.h \
	provider/client/ECMAPIFolderPublic.cpp provider/client/ECMAPIFolderPublic.h \
	provider/client/ECMAPIProp.cpp provider/client/ECMAPIProp.h \
	provider/client/ECMAPITable.cpp provider/client/ECMAPITable.h \
	provider/client/ECMSProvider.cpp provider/client/ECMSProvider.h \
	provider/client/ECMailUser.cpp provider/client/ECMailUser.h \
	provider/client/ECMessage.cpp provider/client/ECMessage.h \
	provider/client/ECMessageStreamImporterIStreamAdapter.cpp \
	provider/client/ECMessageStreamImporterIStreamAdapter.h \
	provider/client/ECMsgStore.cpp provider/client/ECMsgStore.h \
	provider/client/ECMsgStorePublic.cpp provider/client/ECMsgStorePublic.h \
	provider/client/ECNamedProp.cpp provider/client/ECNamedProp.h \
	provider/client/ECNotifyClient.cpp provider/client/ECNotifyClient.h \
	provider/client/ECNotifyMaster.cpp provider/client/ECNotifyMaster.h \
	provider/client/ECParentStorage.cpp provider/client/ECParentStorage.h \
	provider/client/ECPropertyEntry.cpp provider/client/ECPropertyEntry.h \
	provider/client/ECSessionGroupManager.cpp provider/client/ECSessionGroupManager.h \
	provider/client/EntryPoint.h \
	provider/client/IECPropStorage.h \
	provider/client/ProviderUtil.cpp provider/client/ProviderUtil.h \
	provider/client/SessionGroupData.cpp provider/client/SessionGroupData.h \
	provider/client/WSMAPIFolderOps.cpp provider/client/WSMAPIFolderOps.h \
	provider/client/WSMAPIPropStorage.cpp provider/client/WSMAPIPropStorage.h \
	provider/client/WSMessageStreamExporter.cpp provider/client/WSMessageStreamExporter.h \
	provider/client/WSMessageStreamImporter.cpp provider/client/WSMessageStreamImporter.h \
	provider/client/WSSerializedMessage.cpp provider/client/WSSerializedMessage.h \
	provider/client/WSStoreTableView.cpp provider/client/WSStoreTableView.h \
	provider/client/WSTableView.cpp provider/client/WSTableView.h \
	provider/client/WSTransport.cpp provider/client/WSTransport.h \
	provider/client/WSUtil.cpp provider/client/WSUtil.h \
	provider/client/epclient.cpp \
	provider/client/resource.h \
	provider/common/SOAPSock.cpp provider/common/SOAPSock.h \
	provider/soap/soapKCmdProxy.h provider/soap/soapKCmdProxy.cpp
libkcclient_la_LIBADD = \
	libkcutil.la libkcfreebusy.la \
	libmapi.la libkcsoap.la \
	${GSOAP_LIBS} ${ICU_LIBS} ${INTL_LIBS} ${libHX_LIBS} \
	${SSL_LIBS} ${KRB5_LIBS}
libkcclient_la_SYFLAGS = -Wl,--version-script=provider/provider.sym
libkcclient_la_LDFLAGS = ${AM_LDFLAGS} -avoid-version -module \
	${libkcclient_la_SYFLAGS${NO_VSYM}}
EXTRA_libkcclient_la_DEPENDENCIES = provider/provider.sym


#
# libkccontacts
#
libkccontacts_la_SOURCES = \
	provider/contacts/epcontact.cpp provider/contacts/ZCABData.h \
	provider/contacts/ZCABContainer.h provider/contacts/ZCABContainer.cpp \
	provider/contacts/ZCABLogon.h provider/contacts/ZCABLogon.cpp \
	provider/contacts/ZCABProvider.h provider/contacts/ZCABProvider.cpp \
	provider/contacts/ZCMAPIProp.h provider/contacts/ZCMAPIProp.cpp
libkccontacts_la_LIBADD = \
	libkcutil.la libmapi.la ${ICU_LIBS}
libkccontacts_la_SYFLAGS = -Wl,--version-script=provider/provider.sym
libkccontacts_la_LDFLAGS = ${AM_LDFLAGS} -avoid-version -module \
	${libkccontacts_la_SYFLAGS${NO_VSYM}}
EXTRA_libkccontacts_la_DEPENDENCIES = provider/provider.sym


#
# libkcfreebusy
#
libkcfreebusydir = ${includedir}/libfreebusy
libkcfreebusy_HEADERS = \
	libfreebusy/ECEnumFBBlock.h libfreebusy/ECFBBlockList.h \
	libfreebusy/ECFreeBusyData.h libfreebusy/ECFreeBusySupport.h \
	libfreebusy/PublishFreeBusy.h \
	libfreebusy/freebusy.h libfreebusy/freebusyguid.h \
	libfreebusy/freebusytags.h libfreebusy/freebusyutil.h \
	libfreebusy/recurrence.h
libkcfreebusy_la_SOURCES = \
	libfreebusy/ECEnumFBBlock.cpp libfreebusy/ECFBBlockList.cpp \
	libfreebusy/ECFreeBusyData.cpp libfreebusy/ECFreeBusySupport.cpp \
	libfreebusy/ECFreeBusyUpdate.cpp libfreebusy/ECFreeBusyUpdate.h \
	libfreebusy/PublishFreeBusy.cpp \
	libfreebusy/freebusyutil.cpp libfreebusy/recurrence.cpp
libkcfreebusy_la_LIBADD = libkcutil.la libmapi.la
libkcfreebusy_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined \
	${default_SYFLAGS${NO_VSYM}}
EXTRA_libkcfreebusy_la_DEPENDENCIES = ${default_sym}


#
# libkcicalmapi
#
libkcicalmapidir = ${includedir}/icalmapi
libkcicalmapi_HEADERS = \
	libicalmapi/ICalToMAPI.h libicalmapi/MAPIToICal.h \
	libicalmapi/icalcompat.hpp \
	libicalmapi/icalitem.h libicalmapi/icalmem.hpp \
	libicalmapi/icalrecurrence.h libicalmapi/icaluid.h \
	libicalmapi/mapitovcf.hpp \
	libicalmapi/nameids.h libicalmapi/valarm.h \
	libicalmapi/vcftomapi.hpp libicalmapi/vconverter.h \
	libicalmapi/vevent.h libicalmapi/vfreebusy.h libicalmapi/vtimezone.h \
	libicalmapi/vtodo.h
libkcicalmapi_la_SOURCES = \
	libicalmapi/ICalToMAPI.cpp libicalmapi/MAPIToICal.cpp \
	libicalmapi/icalrecurrence.cpp \
	libicalmapi/icaluid.cpp libicalmapi/mapitovcf.cpp \
	libicalmapi/nameids.cpp libicalmapi/valarm.cpp \
	libicalmapi/vcftomapi.cpp libicalmapi/vconverter.cpp \
	libicalmapi/vevent.cpp libicalmapi/vfreebusy.cpp \
	libicalmapi/vtimezone.cpp libicalmapi/vtodo.cpp
libkcicalmapi_la_LIBADD = \
	libkcfreebusy.la libmapi.la libkcutil.la ${ICAL_LIBS}
libkcicalmapi_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined -version-info 0:0:0 \
	${default_SYFLAGS${NO_VSYM}}
EXTRA_libkcicalmapi_la_DEPENDENCIES = ${default_sym}


#
# libkcinetmapi
#
libkcinetmapi_la_SOURCES = \
	inetmapi/ECMapiUtils.cpp inetmapi/ECMapiUtils.h \
	inetmapi/ECVMIMEUtils.cpp inetmapi/ECVMIMEUtils.h \
	inetmapi/MAPISMTPTransport.cpp inetmapi/MAPISMTPTransport.h \
	inetmapi/MAPIToVMIME.cpp inetmapi/MAPIToVMIME.h \
	inetmapi/VMIMEToMAPI.cpp inetmapi/VMIMEToMAPI.h \
	inetmapi/inetmapi.cpp \
	inetmapi/mapiTextPart.cpp inetmapi/mapiTextPart.h \
	inetmapi/serviceRegistration.inl \
	inetmapi/tnef.cpp inetmapi/tnef.h
libkcinetmapi_la_LIBADD = \
	libmapi.la libkcutil.la libkcicalmapi.la \
	${ICONV_LIBS} ${VMIME_LIBS} ${XML2_LIBS}
libkcinetmapi_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined \
	-version-info 0:0:0 ${default_SYFLAGS${NO_VSYM}}
EXTRA_libkcinetmapi_la_DEPENDENCIES = ${default_sym}


#
# libkcpyplug
#
libkcpyplug_la_CPPFLAGS = \
	${AM_CPPFLAGS} -Ispooler ${PYTHON_CFLAGS} -DSWIG_TYPE_TABLE=MAPI
libkcpyplug_la_SOURCES = \
	common/pymem.hpp spooler/PyMapiPlugin.cpp spooler/PyMapiPlugin.h \
	spooler/PythonSWIGRuntime.h
libkcpyplug_la_SYFLAGS = -Wl,--version-script=spooler/libkcpyplug.sym
libkcpyplug_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined -module \
	${libkcpyplug_la_SYFLAGS${NO_VSYM}}
libkcpyplug_la_LIBADD = \
	libkcutil.la libmapi.la ${PYTHON_LIBS}
EXTRA_libkcpyplug_la_DEPENDENCIES = spooler/libkcpyplug.sym


#
# libkcserver
#
libkcserver_la_SOURCES = \
	common/database.cpp common/include/kopano/database.hpp \
	provider/libserver/ECABObjectTable.cpp provider/libserver/ECABObjectTable.h \
	provider/libserver/ECAttachmentStorage.cpp provider/libserver/ECAttachmentStorage.h \
	provider/libserver/ECCacheManager.cpp provider/libserver/ECCacheManager.h \
	provider/libserver/ECConvenientDepthObjectTable.cpp \
	provider/libserver/ECConvenientDepthObjectTable.h \
	provider/libserver/ECDBDef.h \
	provider/libserver/ECDatabase.h \
	provider/libserver/ECDatabaseFactory.cpp provider/libserver/ECDatabaseFactory.h \
	provider/libserver/ECDatabaseMySQL.cpp \
	provider/libserver/ECDatabaseUtils.cpp provider/libserver/ECDatabaseUtils.h \
	provider/libserver/ECGenProps.cpp provider/libserver/ECGenProps.h \
	provider/libserver/ECGenericObjectTable.cpp \
	provider/libserver/ECGenericObjectTable.h \
	provider/libserver/ECICS.cpp provider/libserver/ECICS.h \
	provider/libserver/ECICSHelpers.cpp provider/libserver/ECICSHelpers.h \
	provider/libserver/ECIndexer.cpp provider/libserver/ECIndexer.h \
	provider/libserver/ECKrbAuth.cpp provider/libserver/ECKrbAuth.h \
	provider/libserver/ECLockManager.h provider/libserver/ECMAPI.h \
	provider/libserver/ECNotification.h \
	provider/libserver/ECNotificationManager.cpp provider/libserver/ECNotificationManager.h \
	provider/libserver/ECPluginFactory.cpp provider/libserver/ECPluginFactory.h \
	provider/libserver/ECPluginSharedData.cpp \
	provider/libserver/ECS3Attachment.cpp provider/libserver/ECS3Attachment.h \
	provider/libserver/ECSearchFolders.cpp provider/libserver/ECSearchFolders.h \
	provider/libserver/ECSecurity.cpp provider/libserver/ECSecurity.h \
	provider/libserver/ECServerEntrypoint.cpp provider/libserver/ECServerEntrypoint.h \
	provider/libserver/ECSession.cpp provider/libserver/ECSession.h \
	provider/libserver/ECSessionGroup.cpp provider/libserver/ECSessionGroup.h \
	provider/libserver/ECSessionManager.cpp provider/libserver/ECSessionManager.h \
	provider/libserver/ECStatsTables.cpp provider/libserver/ECStatsTables.h \
	provider/libserver/ECStoreObjectTable.cpp provider/libserver/ECStoreObjectTable.h \
	provider/libserver/ECSubRestriction.cpp provider/libserver/ECSubRestriction.h \
	provider/libserver/ECTPropsPurge.cpp provider/libserver/ECTPropsPurge.h \
	provider/libserver/ECTableManager.cpp provider/libserver/ECTableManager.h \
	provider/libserver/ECTestProtocol.cpp provider/libserver/ECTestProtocol.h \
	provider/libserver/ECUserManagement.cpp provider/libserver/ECUserManagement.h \
	provider/libserver/ECUserStoreTable.cpp provider/libserver/ECUserStoreTable.h \
	provider/libserver/StorageUtil.cpp provider/libserver/StorageUtil.h \
	provider/libserver/StreamUtil.cpp provider/libserver/StreamUtil.h \
	provider/libserver/cmd.cpp provider/libserver/cmd.hpp \
	provider/libserver/cmdutil.cpp provider/libserver/cmdutil.hpp \
	provider/common/ECSearchClient.cpp provider/common/ECSearchClient.h \
	provider/soap/soapKCmdService.h provider/soap/soapKCmdService.cpp
libkcserver_la_LIBADD = \
	libkcutil.la libkcsoap.la -lpthread ${icu_i18n_LIBS} ${icu_uc_LIBS} \
	${GSOAP_LIBS} ${GZ_LIBS} ${kcoidc_LIBS} ${kustomer_LIBS} \
	${KRB5_LIBS} ${libHX_LIBS} ${MYSQL_LIBS} ${PAM_LIBS} ${SSL_LIBS}
libkcserver_la_SYFLAGS = -Wl,--version-script=provider/libkcserver.sym
libkcserver_la_LDFLAGS = ${AM_LDFLAGS} \
	${libkcserver_la_SYFLAGS${NO_VSYM}}
EXTRA_libkcserver_la_DEPENDENCIES = provider/libkcserver.sym

provider/libkcserver.sym: ${top_builddir}/config.status


#
# libkcsoap
#
libkcsoap_la_SOURCES = \
	provider/common/soapctor.cpp provider/soap/soapC.cpp \
	provider/common/pcuser.cpp provider/common/SOAPAlloc.h \
	provider/common/SOAPUtils.cpp provider/common/SOAPUtils.h \
	provider/common/pcutil.cpp provider/common/pcutil.hpp \
	provider/common/versions.h
libkcsoap_la_CXXFLAGS = ${AM_CXXFLAGS} -fvisibility=default
libkcsoap_la_LIBADD = libkcutil.la ${GSOAP_LIBS}
libkcsoap_la_SYFLAGS = -Wl,--version-script=provider/libkcsoap.sym
libkcsoap_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined \
	${libkcsoap_la_SYFLAGS${NO_VSYM}}
EXTRA_libkcsoap_la_DEPENDENCIES = provider/libkcsoap.sym

provider/libkcsoap.sym: ${top_builddir}/config.status


#
# libkcutil
#
noinst_HEADERS += \
	common/ConsoleTable.h common/ECACL.h common/ECCache.h \
	common/ECChannelClient.h \
	common/ECFifoBuffer.h common/ECMemStream.h common/ECSerializer.h \
	common/HtmlEntity.h common/HtmlToTextParser.h common/SSLUtil.h \
	common/StatsClient.h common/rtfutil.h common/charset/localeutil.h
libkcutil_la_SOURCES = \
	common/ConsoleTable.cpp \
	common/ECChannel.cpp common/ECChannelClient.cpp \
	common/ECConfigImpl.cpp common/ECGuid.cpp \
	common/ECKeyTable.cpp common/ECLogger.cpp \
	common/ECMemStream.cpp common/ECThreadPool.cpp \
	common/ECUnknown.cpp common/HtmlEntity.cpp common/HtmlToTextParser.cpp \
	common/MAPIErrors.cpp common/SSLUtil.cpp \
	common/StatsClient.cpp common/TimeUtil.cpp \
	common/UnixUtil.cpp common/fileutil.cpp common/license.cpp \
	common/platform.linux.cpp common/rtfutil.cpp common/stringutil.cpp \
	common/ustringutil.cpp common/charset/convert.cpp \
	common/charset/localeutil.cpp
libkcutil_la_LIBADD = \
	-lpthread -lz ${CRYPTO_LIBS} ${curl_LIBS} ${GZ_LIBS} ${iconv_LIBS} \
	${idn_LIBS} ${icu_i18n_LIBS} ${icu_uc_LIBS} ${INTL_LIBS} \
	${jsoncpp_LIBS} ${libHX_LIBS} \
	${SSL_LIBS} ${UUID_LIBS}
libkcutil_la_SYFLAGS = -Wl,--version-script=common/libkcutil.sym
libkcutil_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined \
	${libkcutil_la_SYFLAGS${NO_VSYM}}
EXTRA_libkcutil_la_DEPENDENCIES = common/libkcutil.sym


common/libkcutil.sym: ${top_builddir}/config.status


#
# libmapi
#
libmapi_la_SOURCES = \
	mapi4linux/src/mapidefs.cpp mapi4linux/src/mapispi.cpp mapi4linux/src/mapiutil.cpp \
	mapi4linux/src/mapix.cpp mapi4linux/src/rtf.cpp \
	mapi4linux/src/m4l.mapidefs.h mapi4linux/src/m4l.mapispi.h \
	mapi4linux/src/m4l.mapix.h mapi4linux/src/rtf.h \
	mapi4linux/src/m4l.mapisvc.cpp mapi4linux/src/m4l.mapisvc.h \
	m4lcommon/CommonUtil.cpp \
	m4lcommon/ECFeatures.cpp m4lcommon/ECFeatureList.h \
	m4lcommon/ECMemTable.cpp m4lcommon/ECRestriction.cpp \
	m4lcommon/RecurrenceState.cpp m4lcommon/userutil.cpp \
	m4lcommon/MAPINotifSink.cpp m4lcommon/MAPINotifSink.h \
	m4lcommon/Util.cpp \
	m4lcommon/mapicontact.h m4lcommon/mapilicense.cpp
libmapi_la_LIBADD = \
	libkcutil.la -lpthread ${DL_LIBS} ${ICONV_LIBS} ${INTL_LIBS}
libmapi_la_SYFLAGS = -Wl,--version-script=mapi4linux/src/libmapi.sym
libmapi_la_LDFLAGS = ${AM_LDFLAGS} -no-undefined -version-info 1:0:0 \
	${libmapi_la_SYFLAGS${NO_VSYM}}
EXTRA_libmapi_la_DEPENDENCIES = mapi4linux/src/libmapi.sym

mapi4linux/src/libmapi.sym: ${top_builddir}/config.status

libm4lcommondir = ${includedir}/mapi4linux
libm4lcommon_HEADERS = m4lcommon/include/edkguid.h


#
#
#
libauthdb_mapi_la_SOURCES = gateway/doveauth1.c gateway/doveauth.cpp
libauthdb_mapi_la_LDFLAGS = -avoid-version -module -Wl,--version-script=${srcdir}/gateway/doveauth.sym
libauthdb_mapi_la_LIBADD  = libmapi.la
EXTRA_libauthdb_mapi_la_DEPENDENCIES = gateway/doveauth.sym

# dovecot expects to find "_plugin" in filenames
libdovemapi_plugin_la_SOURCES = gateway/dovestore1.c gateway/dovestore.cpp
libdovemapi_plugin_la_LDFLAGS = -avoid-version -module -Wl,--version-script=${srcdir}/gateway/dovestore.sym
libdovemapi_plugin_la_LIBADD  = ${libHX_LIBS} libmapi.la
EXTRA_libdovemapi_plugin_la_DEPENDENCIES = gateway/dovestore.sym

gateway/doveauth.sym gateway/dovestore.sym: ${top_builddir}/config.status


#
# PHP configuration
# Old note: Debian has a broken php-config(1) script which outputs
# 	"--with-config-file-scan-dir=/etc/php5/apache2/conf.d"
# If we install our config file into that location, it will only be available
# to only apache2-mod_php5 and not php-cli, php-fpm, etc.
#
phpsysconfdir = ${dcprefix}${PHP_SYSCONF_DIR}
dist_phpsysconf_DATA = php-ext/mapi.ini


#
# PHP extension
#
phplibdir = ${dcprefix}${PHP_EXTENSION_DIR}
mapi_la_CPPFLAGS = ${AM_CPPFLAGS} ${PHP_INCLUDES} -I${srcdir}/php-ext
mapi_la_CXXFLAGS = ${AM_CXXFLAGS} ${ZPHPCXXFLAGS}
if WITH_PHP7
phplib_LTLIBRARIES = mapi.la
endif
mapi_la_SOURCES = \
	php-ext/ECImportContentsChangesProxy.cpp \
	php-ext/ECImportContentsChangesProxy.h \
	php-ext/ECImportHierarchyChangesProxy.cpp \
	php-ext/ECImportHierarchyChangesProxy.h \
	php-ext/ECRulesTableProxy.cpp php-ext/ECRulesTableProxy.h \
	php-ext/globals.h php-ext/main.cpp php-ext/main.h \
	php-ext/phpconfig.h \
	php-ext/typeconversion.cpp php-ext/typeconversion.h \
	php-ext/util.cpp php-ext/util.h
mapi_la_LIBADD = libmapi.la libkcutil.la libkcfreebusy.la \
	libkcinetmapi.la libkcicalmapi.la -lpthread
mapi_la_SYFLAGS = -Wl,--version-script=${srcdir}/php-ext/mapi.sym
mapi_la_LDFLAGS = ${AM_LDFLAGS} -avoid-version -module ${PHP_LDFLAGS} \
	${mapi_la_SYFLAGS${NO_VSYM}}
EXTRA_mapi_la_DEPENDENCIES = ${srcdir}/php-ext/mapi.sym


#
# auxiliary programs
#
eidprint_SOURCES = ECtools/eidprint.cpp
eidprint_LDADD = libkcutil.la libmapi.la
kscriptrun_SOURCES = ECtools/scriptrun.cpp
kscriptrun_LDADD = libkcutil.la
mapitime_SOURCES = ECtools/mapitime.cpp
mapitime_LDADD = ${clock_LIBS} libmapi.la libkcutil.la \
	${curl_LIBS} ${icu_uc_LIBS}
tests_ablookup_SOURCES = tests/ablookup.cpp
tests_ablookup_LDADD = libmapi.la libkcutil.la
tests_htmltext_SOURCES = tests/htmltext.cpp
tests_htmltext_LDADD = libkcutil.la
tests_chtmltotextparsertest_SOURCES = tests/chtmltotextparsertest.cpp
tests_chtmltotextparsertest_LDADD = libkcutil.la
tests_rtfhtmltest_SOURCES = tests/rtfhtmltest.cpp
tests_rtfhtmltest_LDADD = libkcutil.la
tests_imtomapi_SOURCES = tests/imtomapi.cpp tests/tbi.hpp
tests_imtomapi_LDADD = libkcinetmapi.la libmapi.la libkcutil.la
tests_kc_335_SOURCES = tests/kc-335.cpp tests/tbi.hpp
tests_kc_335_LDADD = libmapi.la libkcutil.la
tests_kc_1759_SOURCES = tests/kc-1759.cpp
tests_kc_1759_LDADD = libmapi.la libkcutil.la
tests_mapialloctime_SOURCES = tests/mapialloctime.cpp
tests_mapialloctime_LDADD = libmapi.la ${clock_LIBS}
tests_mapisuite_SOURCES = tests/mapisuite.cpp
tests_mapisuite_LDADD = libmapi.la ${cppunit_LIBS}
tests_readflag_SOURCES = tests/readflag.cpp tests/tbi.hpp
tests_readflag_LDADD = libmapi.la libkcutil.la
tests_ustring_SOURCES = tests/ustring.cpp
tests_ustring_LDADD = libkcutil.la ${icu_uc_LIBS}
tests_zcpmd5_SOURCES = tests/zcpmd5.cpp
tests_zcpmd5_LDADD = ${CRYPTO_LIBS} libkcutil.la


#
# spooler generators
#
BUILT_SOURCES += spooler/PythonSWIGRuntime.h
CLEANFILES += spooler/PythonSWIGRuntime.h
EXTRA_DIST += spooler/PythonSWIGRuntime.h

spooler/PythonSWIGRuntime.h:
if ENABLE_PYBIND
	${SWIG_EXEC} -python -external-runtime $@
else
	: >$@
endif


#
# soap generators
#
SOAPHEADER = provider/include/proto.h
SOAP_OUT = \
	provider/soap/soapC.cpp provider/soap/soapH.h \
	provider/soap/soapKCmdProxy.cpp provider/soap/soapKCmdProxy.h \
	provider/soap/soapKCmdService.cpp provider/soap/soapKCmdService.h \
	provider/soap/soapStub.h
ESOAP_OUT = \
	provider/soap/emptysoapStub.h \
	provider/soap/emptysoapH.h provider/soap/emptysoapC.cpp

${SOAP_OUT}: provider/soap/KCmd.nsmap
${ESOAP_OUT}: provider/soap/soap.nsmap
provider/soap/KCmd.nsmap: ${SOAPHEADER}
	${MKDIR_P} provider/soap
	soapcpp2 -Ed -jxw -Iinclude/ -d provider/soap/ $<
provider/soap/soap.nsmap:
	${MKDIR_P} provider/soap
	soapcpp2 -jxwp emptysoap -d provider/soap/ /dev/null

BUILT_SOURCES += provider/soap/KCmd.nsmap provider/soap/soap.nsmap \
	${SOAP_OUT} ${ESOAP_OUT}
CLEANFILES += ${SOAP_OUT} ${ESOAP_OUT} provider/soap/*


#
# server userdb plugins
#
COMMON_PLUGIN_FILES = provider/plugins/plugin.h
COMMON_DBPLUGIN_FILES = provider/plugins/DBBase.h provider/plugins/DBBase.cpp
pkglib_LTLIBRARIES += libkcserver-db.la
noinst_DATA += libkcserver-db.ldd
libkcserver_db_la_SOURCES = \
	provider/plugins/DBUserPlugin.cpp provider/plugins/DBUserPlugin.h \
	${COMMON_PLUGIN_FILES} ${COMMON_DBPLUGIN_FILES}
libkcserver_db_la_LIBADD = \
	libkcutil.la libkcserver.la ${MYSQL_LIBS}
libkcserver_db_la_LDFLAGS = ${AM_LDFLAGS} -avoid-version -module \
	${plugin_SYFLAGS${NO_VSYM}}
EXTRA_libkcserver_db_la_DEPENDENCIES = provider/plugin.sym

if WITH_UNIX
pkglib_LTLIBRARIES += libkcserver-unix.la
noinst_DATA += libkcserver-unix.ldd
libkcserver_unix_la_SOURCES = \
	provider/plugins/UnixUserPlugin.cpp provider/plugins/UnixUserPlugin.h \
	${COMMON_PLUGIN_FILES} ${COMMON_DBPLUGIN_FILES}
libkcserver_unix_la_LIBADD = \
	libkcutil.la libkcserver.la ${MYSQL_LIBS} ${CRYPT_LIBS}
libkcserver_unix_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version \
	${plugin_SYFLAGS${NO_VSYM}}
EXTRA_libkcserver_unix_la_DEPENDENCIES = provider/plugin.sym
endif

if WITH_LDAP
pkglib_LTLIBRARIES += libkcserver-ldap.la
noinst_DATA += libkcserver-ldap.ldd
libkcserver_ldap_la_SOURCES = \
	provider/plugins/LDAPUserPlugin.cpp provider/plugins/LDAPUserPlugin.h \
	provider/plugins/LDAPCache.cpp provider/plugins/LDAPCache.h \
	provider/plugins/ldappasswords.cpp provider/plugins/ldappasswords.h \
	${COMMON_PLUGIN_FILES}
libkcserver_ldap_la_LIBADD = \
	libkcutil.la \
	libkcserver.la ${CRYPTO_LIBS} ${LDAP_LIBS}
libkcserver_ldap_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version \
	${plugin_SYFLAGS${NO_VSYM}}
EXTRA_libkcserver_ldap_la_DEPENDENCIES = provider/plugin.sym
endif

gdbautoloaddir = ${datadir}/gdb/auto-load/${libdir}
dist_gdbautoload_DATA = tools/libmapi.so.1.0.0-gdb.py

endif # ENABLE_BASE


#
# manual pages
#
dist_man_MANS =
if ENABLE_BASE
dist_man_MANS += \
	doc/kopano.7 \
	doc/kopano-admin.8 doc/kopano-admin.cfg.5 \
	doc/kopano-archiver.8 doc/kopano-archiver.cfg.5 \
	doc/kopano-cfgchecker.8 doc/kopano-coredump.5 \
	doc/kopano-dagent.8 doc/kopano-dagent.cfg.5 doc/kopano-dbadm.8 \
	doc/kopano-fsck.1 \
	doc/kopano-gateway.8 doc/kopano-gateway.cfg.5 doc/kopano-ibrule.1 \
	doc/kopano-ical.8 doc/kopano-ical.cfg.5 \
	doc/kopano-ldap.cfg.5 \
	doc/kopano-monitor.8 doc/kopano-monitor.cfg.5 \
	doc/kopano-oof.1 doc/kopano-passwd.1 \
	doc/kopano-server.8 doc/kopano-server.cfg.5 \
	doc/kopano-spooler.8 doc/kopano-spooler.cfg.5 \
	doc/kopano-statsd.8 doc/kopano-statsd.cfg.5 \
	doc/kopano-srvadm.8 doc/kopano-stats.1 doc/kopano-storeadm.8 \
	doc/kopano-unix.cfg.5 doc/kopano-vcfimport.1 \
	doc/mapi.7
endif
if ENABLE_PYTHON
dist_man_MANS += \
	doc/kopano-archiver-aclset.8 \
	doc/kopano-archiver-aclsync.8 \
	doc/kopano-archiver-restore.8 \
	doc/kopano-autorespond.8 \
	doc/kopano-backup.8 doc/kopano-backup.cfg.5 \
	doc/kopano-cachestat.8 \
	doc/kopano-mailbox-permissions.8 \
	doc/kopano-mr-accept.8 doc/kopano-mr-process.8 \
	doc/kopano-search.8 doc/kopano-search.cfg.5 \
	doc/kopano-spamd.8 doc/kopano-spamd.cfg.5
endif

#
# build infrastructure
#
default.sym provider/plugin.sym provider/provider.sym spooler/libkcpyplug.sym: ${top_builddir}/config.status

include global.am


EXTRA_DIST += versionheader.sh \
	AGPL-3 LICENSE.txt RELNOTES.txt \
	bootstrap.sh lddcheck global.am pwdcheck.sh

BUILT_SOURCES += source-moved-check common/include/kopano/ecversion.h
CLEANFILES += common/include/kopano/ecversion.h

AM_DISTCHECK_CONFIGURE_FLAGS = --with-dcprefix='$${prefix}'

.PHONY: source-moved-check
source-moved-check:
	@${top_srcdir}/pwdcheck.sh "${abs_top_builddir}"

.version:
	echo "${PACKAGE_VERSION}" >$@

common/include/kopano/ecversion.h: .version versionheader.sh
	${MKDIR_P} common/include/kopano
	sh ${top_srcdir}/versionheader.sh >$@

#
# test suite entry point
#
.PHONY: test
test:
	$(MAKE) -C test test

.PHONY: test-short
test-short:
	$(MAKE) -C test test-short

.PHONY: test-ci
test-ci:
	$(MAKE) -C test test-ci
