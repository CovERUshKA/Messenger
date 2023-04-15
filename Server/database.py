from pymongo import MongoClient

# Provide the mongodb atlas url to connect python to mongodb using pymongo
CONNECTION_STRING = "mongodb://localhost:27017"

# Create a connection using MongoClient. You can import MongoClient or use pymongo.MongoClient
client = MongoClient(CONNECTION_STRING)

# db = TinyDB('db.json')

# User = Query()

# users = db.table("users")

# chats = db.table("chats")

# lock = threading.Lock()

#lock = threading.Lock()

# Подключаемся к файлу базы данных
#con = sqlite3.connect("main.db", check_same_thread=False)
# Берём управление базой данных
#cur = con.cursor()

# Создаём таблицу пользователей, если не создана
#cur.execute("create table if not exists users (user_id INTEGER PRIMARY KEY AUTOINCREMENT, username VARCHAR(32) UNIQUE NOT NULL, password VARCHAR(32) NOT NULL)")

# Создаём таблицу чатов, если не создана
#cur.execute("create table if not exists chats (chat_id INTEGER PRIMARY KEY AUTOINCREMENT, type VARCHAR(32) UNIQUE NOT NULL, password VARCHAR(32) NOT NULL)")
