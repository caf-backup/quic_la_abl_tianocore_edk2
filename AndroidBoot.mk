#Android makefile to build bootloader as a part of Android Build
ANDROID_TOP=$(shell pwd)
CLANG_BIN := $(ANDROID_TOP)/$(LLVM_PREBUILTS_PATH)/
ABL_USE_SDLLVM := false

ifneq ($(FORCE_SDCLANG_OFF),true)
ifneq ($(wildcard $(SDCLANG_PATH)),)
  ifeq ($(shell echo $(SDCLANG_PATH) | head -c 1),/)
    CLANG_BIN := $(SDCLANG_PATH)/
  else
    CLANG_BIN := $(ANDROID_TOP)/$(SDCLANG_PATH)/
  endif

  ABL_USE_SDLLVM := true
endif
endif

# LD & make are not available in prebuilts for older Android versions
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) > 27 ))" )))
LDOPT="-fuse-ld=$(ANDROID_TOP)/$(SOONG_LLVM_PREBUILTS_PATH)/ld.lld"
MAKEPATH=$(ANDROID_TOP)/prebuilts/build-tools/linux-x86/bin/
  ifeq (,$(wildcard $(MAKEPATH)make))
    MAKEPATH :=
  endif
endif

# Use host tools from prebuilts. Partner should determine the correct host tools to use
PREBUILT_HOST_TOOLS := CC=$(ANDROID_TOP)/$(CLANG)\ \
		       CXX=$(ANDROID_TOP)/$(CLANG_CXX)\ \
		       LDPATH=$(LDOPT)\ \
		       AR=$(ANDROID_TOP)/$(HOST_AR)
PREBUILT_PYTHON_PATH=$(ANDROID_TOP)/prebuilts/python/linux-x86/2.7.5/bin/python2

DISABLE_PARALLEL_DOWNLOAD_FLASH := DISABLE_PARALLEL_DOWNLOAD_FLASH=0
ifeq ($(PRODUCT_SUPPORTS_VERITY),true)
	VERIFIED_BOOT := VERIFIED_BOOT=1
else
	VERIFIED_BOOT := VERIFIED_BOOT=0
endif

ifeq ($(IS_EARLY_ETH_ENABLED), 1)
	EARLY_ETH_ENABLED := EARLY_ETH_ENABLED=1
else
	EARLY_ETH_ENABLED := EARLY_ETH_ENABLED=0
endif

ifeq ($(BOARD_BUILD_SYSTEM_ROOT_IMAGE),true)
        BUILD_SYSTEM_ROOT_IMAGE := BUILD_SYSTEM_ROOT_IMAGE=1
else
        BUILD_SYSTEM_ROOT_IMAGE := BUILD_SYSTEM_ROOT_IMAGE=0
endif

ifeq ($(BOARD_AVB_ENABLE),true)
	VERIFIED_BOOT_2 := VERIFIED_BOOT_2=1
else
	VERIFIED_BOOT_2 := VERIFIED_BOOT_2=0
endif

ifeq ($(BOARD_LEVB_ENABLE),true)
	VERIFIED_BOOT_LE := VERIFIED_BOOT_LE=1
else
	VERIFIED_BOOT_LE := VERIFIED_BOOT_LE=0
endif

ifeq ($(TARGET_AB_RETRYCOUNT_DISABLE),true)
	AB_RETRYCOUNT_DISABLE := AB_RETRYCOUNT_DISABLE=1
else
	AB_RETRYCOUNT_DISABLE := AB_RETRYCOUNT_DISABLE=0
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
	USER_BUILD_VARIANT := USER_BUILD_VARIANT=1
else
	USER_BUILD_VARIANT := USER_BUILD_VARIANT=0
endif

ifneq ($(TARGET_BOOTLOADER_BOARD_NAME),)
	BOARD_BOOTLOADER_PRODUCT_NAME := $(TARGET_BOOTLOADER_BOARD_NAME)
else
	BOARD_BOOTLOADER_PRODUCT_NAME := QC_Reference_Phone
endif

ifneq ($(TARGET_BOARD_TYPE),auto)
	TARGET_BOARD_TYPE_AUTO := TARGET_BOARD_TYPE_AUTO=0
else
	TARGET_BOARD_TYPE_AUTO := TARGET_BOARD_TYPE_AUTO=1
endif
ifeq ($(BOARD_ABL_SAFESTACK_DISABLE),true)
	ABL_SAFESTACK := false
else
	ABL_SAFESTACK := true
endif

ifeq ($(PRODUCT_USE_DYNAMIC_PARTITIONS),true)
        DYNAMIC_PARTITION_SUPPORT := DYNAMIC_PARTITION_SUPPORT=1
else
        DYNAMIC_PARTITION_SUPPORT := DYNAMIC_PARTITION_SUPPORT=0
endif

ifeq ($(PRODUCT_VIRTUAL_AB_OTA),true)
        VIRTUAL_AB_OTA := VIRTUAL_AB_OTA=1
else
        VIRTUAL_AB_OTA := VIRTUAL_AB_OTA=0
endif

ifeq ($(BOARD_USES_RECOVERY_AS_BOOT),true)
	BUILD_USES_RECOVERY_AS_BOOT := BUILD_USES_RECOVERY_AS_BOOT=1
else
	BUILD_USES_RECOVERY_AS_BOOT := BUILD_USES_RECOVERY_AS_BOOT=0
endif

ifeq ($(TARGET_HIBERNATION_INSECURE_ENABLE),true)
	HIBERNATION_SUPPORT_INSECURE := HIBERNATION_SUPPORT_INSECURE=1
	HIBERNATION_SUPPORT_SECURE := HIBERNATION_SUPPORT_SECURE=0
else
	HIBERNATION_SUPPORT_INSECURE := HIBERNATION_SUPPORT_INSECURE=0
endif

ifeq ($(TARGET_HIBERNATION_SECURE_ENABLE),true)
	HIBERNATION_SUPPORT_INSECURE := HIBERNATION_SUPPORT_INSECURE=1
	HIBERNATION_SUPPORT_SECURE := HIBERNATION_SUPPORT_SECURE=1
else
	HIBERNATION_SUPPORT_SECURE := HIBERNATION_SUPPORT_SECURE=0
endif

ifeq ($(TARGET_LINUX_BOOT_CPU_SELECTION),true)
	LINUX_BOOT_CPU_SELECTION_ENABLED := LINUX_BOOT_CPU_SELECTION_ENABLED=1
else
	LINUX_BOOT_CPU_SELECTION_ENABLED := LINUX_BOOT_CPU_SELECTION_ENABLED=0
endif

ifneq ($(TARGET_LINUX_BOOT_CPU_ID),)
	TARGET_LINUX_BOOT_CPU_ID := TARGET_LINUX_BOOT_CPU_ID=$(TARGET_LINUX_BOOT_CPU_ID)
else
	TARGET_LINUX_BOOT_CPU_ID := TARGET_LINUX_BOOT_CPU_ID=0
endif

SAFESTACK_SUPPORTED_CLANG_VERSION = 6.0

# For most platform, abl needed always be built
# in aarch64 arthitecture to run.
# Specify BOOTLOADER_ARCH if needed to built with
# other ARCHs.
ifeq ($(BOOTLOADER_ARCH),)
	BOOTLOADER_ARCH := AARCH64
endif
TARGET_ARCHITECTURE := $(BOOTLOADER_ARCH)

ifeq ($(TARGET_ARCHITECTURE),arm)
	CLANG35_PREFIX := $(ANDROID_TOP)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-$(TARGET_GCC_VERSION)/bin/arm-linux-androideabi-
	CLANG35_GCC_TOOLCHAIN := $(ANDROID_TOP)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-$(TARGET_GCC_VERSION)
else
	CLANG35_PREFIX := $(ANDROID_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-$(TARGET_GCC_VERSION)/bin/aarch64-linux-android-
	CLANG35_GCC_TOOLCHAIN := $(ANDROID_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-$(TARGET_GCC_VERSION)
endif

# SECIMAGE_BASE : sectools folder path
ifneq ($(wildcard $(QCPATH)/sectools),)
   SECIMAGE_BASE := $(QCPATH)/sectools
else
   SECIMAGE_BASE := $(QCPATH)/common/scripts/SecImage
endif

ifeq ($(USE_SOC_HW_VERSION), true)
   soc_hw_version = $(SOC_HW_VERSION)
   soc_vers = $(SOC_VERS)
endif

SIGN_ID := abl

ifeq ($(USE_SOC_HW_VERSION), true)
  # XML_FILE : this file will be used for singing abl and it might be different for each target
   ifeq ($(SECIMAGE_BASE), $(QCPATH)/sectools)
      ifeq ($(call is-board-platform-in-list,lahaina holi),true)
        XML_FILE := secimage_eccv3.xml
      else
        XML_FILE := secimagev3.xml
      endif
   else
     XML_FILE := secimagev2.xml
   endif
   define sec-image-generate
	@echo Generating signed appsbl using secimage tool for $(strip $(QTI_GENSECIMAGE_MSM_IDS))
	@rm -rf $(PRODUCT_OUT)/signed
	$(hide) SECIMAGE_LOCAL_DIR=$(SECIMAGE_BASE) USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN=$(USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN) \
			USES_SEC_POLICY_DEFAULT_SUBFOLDER_SIGN=$(USES_SEC_POLICY_DEFAULT_SUBFOLDER_SIGN) \
			USES_SEC_POLICY_INTEGRITY_CHECK=$(USES_SEC_POLICY_INTEGRITY_CHECK) python $(SECIMAGE_BASE)/sectools_builder.py \
		-i $(TARGET_EMMC_BOOTLOADER) \
		-t $(PRODUCT_OUT)/signed \
		-g $(SIGN_ID) \
		--soc_hw_version $(soc_hw_version) \
                --soc_vers $(soc_vers) \
                --config=$(SECIMAGE_BASE)/config/integration/$(XML_FILE) \
		--install_base_dir=$(PRODUCT_OUT) \
		 > $(PRODUCT_OUT)/secimage.log 2>&1
	@echo Completed secimage signed appsbl \(logs in $(PRODUCT_OUT)/secimage.log\)
   endef
else
   define sec-image-generate
        @echo Generating signed appsbl using secimage tool for $(strip $(QTI_GENSECIMAGE_MSM_IDS))
        @rm -rf $(PRODUCT_OUT)/signed
        $(hide) SECIMAGE_LOCAL_DIR=$(SECIMAGE_BASE) USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN=$(USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN) \
                        USES_SEC_POLICY_DEFAULT_SUBFOLDER_SIGN=$(USES_SEC_POLICY_DEFAULT_SUBFOLDER_SIGN) \
                        USES_SEC_POLICY_INTEGRITY_CHECK=$(USES_SEC_POLICY_INTEGRITY_CHECK) python $(SECIMAGE_BASE)/sectools_builder.py \
                -i $(TARGET_EMMC_BOOTLOADER) \
                -t $(PRODUCT_OUT)/signed \
                -g $(SIGN_ID) \
                --config=$(SECIMAGE_BASE)/config/integration/secimage.xml \
                --install_base_dir=$(PRODUCT_OUT) \
                 > $(PRODUCT_OUT)/secimage.log 2>&1
        @echo Completed secimage signed appsbl \(logs in $(PRODUCT_OUT)/secimage.log\)
   endef
endif

# ABL ELF output
TARGET_ABL := $(PRODUCT_OUT)/abl.elf
TARGET_EMMC_BOOTLOADER := $(TARGET_ABL)
ABL_OUT := $(TARGET_OUT_INTERMEDIATES)/ABL_OBJ

$(ABL_OUT):
	mkdir -p $(ABL_OUT)

# Top level target
LOCAL_ABL_PATH := bootable/bootloader/edk2
LOCAL_ABL_SRC_FILE := $(shell find $(LOCAL_ABL_PATH) -name "*" -type f | sed  "s%\.\/%$(LOCAL_ABL_PATH)\/%g")
$(TARGET_ABL): $(LOCAL_ABL_SRC_FILE) | $(ABL_OUT) $(INSTALLED_KEYSTOREIMAGE_TARGET)
	$(MAKEPATH)make -C bootable/bootloader/edk2 \
		BOOTLOADER_OUT=../../../$(ABL_OUT) \
		all \
		PREBUILT_HOST_TOOLS=$(PREBUILT_HOST_TOOLS) \
		PREBUILT_PYTHON_PATH=$(PREBUILT_PYTHON_PATH) \
		MAKEPATH=$(MAKEPATH) \
		$(BUILD_SYSTEM_ROOT_IMAGE) \
		$(VERIFIED_BOOT) \
		$(VERIFIED_BOOT_2) \
		$(VERIFIED_BOOT_LE) \
		$(EARLY_ETH_ENABLED) \
		$(USER_BUILD_VARIANT) \
		$(DISABLE_PARALLEL_DOWNLOAD_FLASH) \
		$(AB_RETRYCOUNT_DISABLE) \
		$(HIBERNATION_SUPPORT_INSECURE) \
		$(HIBERNATION_SUPPORT_SECURE) \
		$(DYNAMIC_PARTITION_SUPPORT) \
		$(TARGET_BOARD_TYPE_AUTO) \
		$(VIRTUAL_AB_OTA) \
		$(BUILD_USES_RECOVERY_AS_BOOT) \
		$(LINUX_BOOT_CPU_SELECTION_ENABLED) \
		$(TARGET_LINUX_BOOT_CPU_ID) \
		CLANG_BIN=$(CLANG_BIN) \
		CLANG_PREFIX=$(CLANG35_PREFIX)\
		ABL_USE_SDLLVM=$(ABL_USE_SDLLVM) \
		ABL_SAFESTACK=$(ABL_SAFESTACK) \
		SAFESTACK_SUPPORTED_CLANG_VERSION=$(SAFESTACK_SUPPORTED_CLANG_VERSION) \
		CLANG_GCC_TOOLCHAIN=$(CLANG35_GCC_TOOLCHAIN)\
		TARGET_ARCHITECTURE=$(TARGET_ARCHITECTURE) \
		BOARD_BOOTLOADER_PRODUCT_NAME=$(BOARD_BOOTLOADER_PRODUCT_NAME)

SIGN_ABL := $(PRODUCT_OUT)/temp_signed
$(SIGN_ABL) : $(TARGET_EMMC_BOOTLOADER)
	$(hide) $(call sec-image-generate)
