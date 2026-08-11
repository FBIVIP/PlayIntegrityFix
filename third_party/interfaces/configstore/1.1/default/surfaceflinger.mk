
LOCAL_SRC_FILES += SurfaceFlingerConfigs.cpp

ifneq ($(VSYNC_EVENT_PHASE_OFFSET_NS),)
    $(call soong_config_set,surfaceflinger,vsync_event_phase_offset_ns,$(VSYNC_EVENT_PHASE_OFFSET_NS))
    LOCAL_CFLAGS += -DVSYNC_EVENT_PHASE_OFFSET_NS=$(VSYNC_EVENT_PHASE_OFFSET_NS)
endif

ifneq ($(SF_VSYNC_EVENT_PHASE_OFFSET_NS),)
    $(call soong_config_set,surfaceflinger,sf_vsync_event_phase_offset_ns,$(SF_VSYNC_EVENT_PHASE_OFFSET_NS))
    LOCAL_CFLAGS += -DSF_VSYNC_EVENT_PHASE_OFFSET_NS=$(SF_VSYNC_EVENT_PHASE_OFFSET_NS)
endif

ifeq ($(TARGET_USE_CONTEXT_PRIORITY),true)
    $(call soong_config_set_bool,surfaceflinger,use_context_priority,true)
    LOCAL_CFLAGS += -DUSE_CONTEXT_PRIORITY=1
endif

ifeq ($(TARGET_HAS_WIDE_COLOR_DISPLAY),true)
    $(call soong_config_set_bool,surfaceflinger,has_wide_color_display,true)
    LOCAL_CFLAGS += -DHAS_WIDE_COLOR_DISPLAY
endif

ifeq ($(TARGET_HAS_HDR_DISPLAY),true)
    $(call soong_config_set_bool,surfaceflinger,has_hdr_display,true)
    LOCAL_CFLAGS += -DHAS_HDR_DISPLAY
endif

ifneq ($(PRESENT_TIME_OFFSET_FROM_VSYNC_NS),)
    $(call soong_config_set,surfaceflinger,present_time_offset_from_vsync_ns,$(PRESENT_TIME_OFFSET_FROM_VSYNC_NS))
    LOCAL_CFLAGS += -DPRESENT_TIME_OFFSET_FROM_VSYNC_NS=$(PRESENT_TIME_OFFSET_FROM_VSYNC_NS)
else
    $(call soong_config_set,surfaceflinger,present_time_offset_from_vsync_ns,0)
    LOCAL_CFLAGS += -DPRESENT_TIME_OFFSET_FROM_VSYNC_NS=0
endif

ifeq ($(TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS),true)
    $(call soong_config_set_bool,surfaceflinger,force_hwc_copy_for_virtual_displays,true)
    LOCAL_CFLAGS += -DFORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
endif

ifneq ($(MAX_VIRTUAL_DISPLAY_DIMENSION),)
    $(call soong_config_set,surfaceflinger,max_virtual_displat_dimension,$(MAX_VIRTUAL_DISPLAY_DIMENSION))
    LOCAL_CFLAGS += -DMAX_VIRTUAL_DISPLAY_DIMENSION=$(MAX_VIRTUAL_DISPLAY_DIMENSION)
endif

ifeq ($(TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK),true)
    $(call soong_config_set_bool,surfaceflinger,running_without_sync_framework,true)
    LOCAL_CFLAGS += -DRUNNING_WITHOUT_SYNC_FRAMEWORK
endif

ifneq ($(USE_VR_FLINGER),)
    $(call soong_config_set_bool,surfaceflinger,use_vr_flinger,true)
    LOCAL_CFLAGS += -DUSE_VR_FLINGER
endif

ifneq ($(NUM_FRAMEBUFFER_SURFACE_BUFFERS),)
    $(call soong_config_set,surfaceflinger,num_framebuffer_surface_buffers,$(NUM_FRAMEBUFFER_SURFACE_BUFFERS))
    LOCAL_CFLAGS += -DNUM_FRAMEBUFFER_SURFACE_BUFFERS=$(NUM_FRAMEBUFFER_SURFACE_BUFFERS)
endif

ifneq ($(SF_START_GRAPHICS_ALLOCATOR_SERVICE),)
    $(call soong_config_set_bool,surfaceflinger,sf_start_graphics_allocator_service,true)
    LOCAL_CFLAGS += -DSTART_GRAPHICS_ALLOCATOR_SERVICE
endif

ifneq ($(SF_PRIMARY_DISPLAY_ORIENTATION),)
    $(call soong_config_set,surfaceflinger,sf_primary_display_orientation,$(SF_PRIMARY_DISPLAY_ORIENTATION))
    LOCAL_CFLAGS += -DPRIMARY_DISPLAY_ORIENTATION=$(SF_PRIMARY_DISPLAY_ORIENTATION)
endif
