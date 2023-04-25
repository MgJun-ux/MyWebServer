# 建议在需要表示内存大小、数组索引等场景下使用 size_t 类型
## 创建自己的数据库

// 建立yourdb库
    create database serveruser;

// 创建user表
    USE serveruser;
    CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
    INSERT INTO user(username, password) VALUES('name', 'password');

## 这个是用github上大佬的代码，我在这里面写了一些注释，用cmake启动