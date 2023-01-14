import threading
from tinydb import TinyDB, Query

db = TinyDB('db.json')

User = Query()

users = db.table("users")

chats = db.table("chats")

lock = threading.Lock()

#lock = threading.Lock()

# Подключаемся к файлу базы данных
#con = sqlite3.connect("main.db", check_same_thread=False)
# Берём управление базой данных
#cur = con.cursor()

# Создаём таблицу пользователей, если не создана
#cur.execute("create table if not exists users (user_id INTEGER PRIMARY KEY AUTOINCREMENT, username VARCHAR(32) UNIQUE NOT NULL, password VARCHAR(32) NOT NULL)")

# Создаём таблицу чатов, если не создана
#cur.execute("create table if not exists chats (chat_id INTEGER PRIMARY KEY AUTOINCREMENT, type VARCHAR(32) UNIQUE NOT NULL, password VARCHAR(32) NOT NULL)")

def execute_and_commit(sql, parameters = ()):
    """Выполняем sql команду и сразу возвращаем результат"""
    """try:
        lock.acquire(True)

        cur.execute(sql, parameters)
        con.commit()
    finally:
        lock.release()"""

    #users.insert({"username":})

def execute_and_fetchall(sql, parameters = ()):
    """Выполняем sql команду и сразу сохраняем её в файл"""
    try:
        lock.acquire(True)

        cur.execute(sql, parameters)
        ret = cur.fetchall()
    finally:
        lock.release()

    return ret

def get_bots():
    """Возвращает список всех ботов в базе данных"""

    bots = execute_and_fetchall("select * from bots")

    return bots

def get_bot_info(name):
    """Возвращает информацию по боту с ником name"""

    if not bot_exist(name):
        return None

    bot = execute_and_fetchall("select * from bots where name like ?", (name,))

    return bot[0]

def bot_exist(name):
    """Проверяет есть ли бот с ником name в базе данных"""

    bots = execute_and_fetchall("select * from bots")

    for i in bots:
        if i[0] == name:
            return True

    return False

def add_bot(name, cookie, accessCode):
    execute_and_commit("INSERT or REPLACE into bots values (?, ?, ?)", (name, cookie, accessCode))

    return True

def remove_bot(name):
    if bot_exist(name):
        execute_and_commit("delete from bots where name like (?)", (name,))
        return True
    else:
        return False