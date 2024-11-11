
link_libraries(${circle_home}/addon/wlan/libwlan.a
${circle_home}/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a
${circle_home}/lib/net/libnet.a
${circle_home}/lib/sched/libsched.a)
add_circle_lib(wlan main.cpp)

# install(DIRECTORY ${circle_home}/addon/wlan/firmware
#         DESTINATION ${CMAKE_INSTALL_PREFIX}/ramdisk/wlan)
# install(FILES wpa_supplicant.conf
#         DESTINATION ${CMAKE_INSTALL_PREFIX}/ramdisk/wlan)

file(COPY ${circle_home}/addon/wlan/firmware/ DESTINATION ${CHCORE_RAMDISK_DIR}/wlan/firmware)
file(COPY wpa_supplicant.conf DESTINATION ${CHCORE_RAMDISK_DIR}/wlan)