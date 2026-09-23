# Database
plain-text relational database

## Product

competes: recutils, SQL, mongodb

interacts: file system, clients, text editors

constitutes: a set of CSV files

includes: config, schema, data

resembles: recutils

patterns: 

stakeholders: fetsorn

## Technical
### 宣 胞 闹 
- user must view dataset in a text editor
### 众 脱 幻
- user must version control the dataset
### 叔 纹 岩
- user must query with grep
### 在 赏 顶
- user must deduplicate values
### 朗 归 娘
- user without a client should figure out the dataset structure
### 波 精 祸
- developer without a client should figure out how to restore data
### 据 叫 妻
- developer must implement searches
### 关 障 讯
- user must specify relations between entities
### 裂 捞 红
- user must store arrays, lists
### 奥 虚 昌
- user must store records, sets
### 良 送 辖
- user must store identical data in efficient space
### 碱 碱 烷
- user should specify foreign keys as sha256sum hashes of values
### 复 究 臂
- user should specify foreign keys as multiformats hashes of values
### 卖 践 杀
- user should store relative values
### 泡 辟 止
- user should store escaped string
### 模 桂 红
- user should store without duplication
### 据 归 改
- user must store values
### 亦 龙 皱
- developer must extend csvs format
### 敌 爬 鬼
- user must read plain text
### 磁 西 催
- user must read dataset files
### 差 纳 父
- user must write dataset files
### 税 七 汽
- developer must easily reverse engineer the dataset structure without documentation
### 决 胀 我
- developer must easily support and maintain the dataset

# Library
library for interacting with csvs datasets

## Product

competes: libraries for SQL, MongoDB, 

interacts: with arbitrary dataset storage that implements FS

constitutes: a library, a WASM reactor module

includes: a façade interface, query controller, record update controller, record delete controller, schema controller, dataset cache

patterns: façade

resembles: 

stakeholders: fetsorn

## Technical

### 龄 摸 律
- user must search records in a csvs dataset
### 馏 温 佛
- user must specify custom read method
### 邻 津 二
- user must specify custom write method
### 大 冶 横
- user must specify custom grep method
### 巡 尾 造
- user must edit record in a csvs dataset
### 沸 油 诚
- user must delete record from a csvs dataset
### 理 双 景
- user must add record to a csvs dataset
### 绝 项 额
- user can optimize csvs dataset files
### 糊 变 审
- user must stream query results
### 迹 黄 虽
- user should find 100k records in under 1 second
### 巩 必 扎
- user must add records in bulk
### 津 飞 妹
- user must edit records in bulk
### 阻 愿 夫
- user must delete records in bulk
### 黄 苯 层
- user must search all keys
### 铅 射 写
- user must build a record from key
### 核 舒 骨
- user must search dataset with multiple trunks
### 似 脚 放
- user must consistently reproduce dataset contents

# CLI
command-line utility for interacting with csvs datasets

## Product

competes: prsql, sqlite

interacts: filesystem

constitutes: a cli utility

includes: 

patterns: 

resembles: 

stakeholders: fetsorn

## Technical
## 北 翻 蚀
- user can create an empty csvs dataset
## 玩 矿 仓
- user must create an empty csvs dataset
## 薄 刺 内
- user must edit an empty csvs dataset
## 夹 轨 检
- user must edit schema of a csvs dataset
## 升 辨 良
- user must delete schema entry dataset
## 氯 弃 检
- user must query entry dataset
