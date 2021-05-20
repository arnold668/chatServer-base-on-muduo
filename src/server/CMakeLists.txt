#定义一个SRC——LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
#加入db
aux_source_directory(./db DB_LIST)

aux_source_directory(./model MODEL_LIST)

aux_source_directory(./redis REDIS_LIST)
#指定生成文件可执行
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
#链接依赖库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient  hiredis pthread)