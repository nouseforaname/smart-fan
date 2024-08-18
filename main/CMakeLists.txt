idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(
  SRCS 
    "main.c"
    "ota/ota.c"
    "wifi/wifi.c"
    "server/server.c"
    "persistence/persistence.c"
  REQUIRES 
  INCLUDE_DIRS 
    "." "ota" "server" "wifi" "common"
  EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem
)
