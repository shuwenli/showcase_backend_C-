
/* $(VROOT)/mlm/src/com/rules.mk */

include $(VROOT)/FMSrules.mk

/* include $(VROOT)/rules/strip.mk */
/* include $(VROOT)/rules/copy.mk  */

.SOURCE.C : $(VROOT)/mlm/src/com
.SOURCE.h : $(VROOT)/mlm/hdr
		$(VROOT)/hdr

/* .SOURCE.so : $(VROOT)/lib/$(MACHINE) */
/* .SOURCE.so : $(VROOT)/mlm/src/$(MACHINE) */

/* include $(VROOT)/rules/nmakefix.mk */

MLM_SRC = MLMConfiguration.C
MLM_SRC += MLMLogger.C
MLM_SRC += MLMProcess.C
MLM_SRC += MLMPIDScanner.C
MLM_SRC += MLMMemMonitor.C
MLM_SRC += MLMMain.C
MLM_SRC += MLMNotification.C
MLM_SRC += MLMAlarm.C
MLM_SRC += MLMDynLib.C
MLM_SRC += MLMConstants.C

MLM_LIB_SRC = MLMEventHandler.C
MLM_TEST_SRC = MLMLeaky.C
MLM_ALGO_SRC =  MLMAlgo.C 

PRODUCT = libmlmfoam.so FLXmlm MLMalgo Leaky

if "$(FMSLINT)" == "flexelint"
	RUNTARGET = flexelint
else
	RUNTARGET = $(PRODUCT)
	if "$(FMSLINT)" == "all"
		RUNTARGET += flexelint
	end
end
:ALL: $(RUNTARGET)

/*
TLIB   = -lpthread
POSLIB = -lposix4
DYNLIB = -ldl
*/

CCFLAGS += -g
CCFLAGS += -DAPXAP
NETLIB = -lsocket
RCC_LIB = -lrccadm -lrccer -lWD -lrccss -lrccmsg -lwdmsg 

$(BINDIR) :INSTALLDIR: $(PRODUCT)

/* nmakefix :: nmakefix.c */

/* FLXmlm  ::  $(MLM_SRC) $(TLIB) $(NETLIB) $(DYNLIB) $(MORELIBS) */

FLXmlm  ::  $(MLM_SRC) $(NETLIB) $(RCC_LIB) -ldl
MLMalgo ::  $(MLM_ALGO_SRC)
Leaky   ::  $(MLM_TEST_SRC)
libmlmfoam.so :: $(MLM_LIB_SRC) $(NETLIB) $(RCC_LIB) -lev

flexelint : flexelint_FLXmlm
flexelint_FLXmlm : .RUNFLEXCP $(MLM_SRC)
flexelint : flexelint_MLMalgo
flexelint_MLMalgo : .RUNFLEXCP $(MLM_ALGO_SRC)
flexelint : flexelint_Leaky
flexelint_Leaky : .RUNFLEXCP $(MLM_TEST_SRC)
flexelint : flexelint_libmlmfoam
flexelint_libmlmfoam : .RUNFLEXCP $(MLM_LIB_SRC)
