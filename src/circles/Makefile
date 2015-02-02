#
# Skin Makefile
#
DUMMY_TARGET=built
TARGET_FRAMEBUFFER?=bgra

$(DUMMY_TARGET): src_images
	rm -rf $(TARGET_FRAMEBUFFER)_dds
	$(SHELL) build_dds.sh $(TARGET_FRAMEBUFFER)
	touch built

src_images:
	@echo "--- Unpacking DDS source images."
	for i in xa*.tfz; do tar xzf $i; done

clean:
	rm -rf $(TARGET_FRAMEBUFFER)_dds src_images built
