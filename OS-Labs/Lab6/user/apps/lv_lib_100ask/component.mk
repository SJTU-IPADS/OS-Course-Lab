# ESP-IDF component file for make based commands

COMPONENT_SRCDIRS := . \
                  src \
                  src/lv_100ask_calc \
				  src/lv_100ask_pinyin_ime \
				  src/lv_100ask_memory_game \
				  src/lv_100ask_page_manager \
				  src/lv_100ask_sketchpad	\
				  src/lv_100ask_2048	\
				  src/lv_100ask_file_explorer	\
				  test \
                  test/lv_100ask_calc_test \
				  test/lv_100ask_pinyin_ime_test \
				  test/lv_100ask_memory_game_test \
				  test/lv_100ask_page_manager_test \
				  test/lv_100ask_sketchpad_test\
				  test/lv_100ask_2048_test\
				  test/lv_100ask_file_explorer


COMPONENT_ADD_INCLUDEDIRS := $(COMPONENT_SRCDIRS) .