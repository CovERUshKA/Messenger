import chats
import database as db

from tinydb import Query

users = db.users

User = Query()

lock = db.lock

def get_user(user_id):
    try:
        lock.acquire(True)

        user = users.search(User.user_id == user_id)
    finally:
        lock.release()

    if len(user) == 0:
        return None

    return user[0]

def get_user_name(user_id):
    user = get_user(user_id)

    if user == None:
        return None
    else:
        username = user["username"]
        return username

def get_user_with_username(username):
    try:
        lock.acquire(True)

        user = users.search(User.username == username)
    finally:
        lock.release()

    if len(user) == 0:
        return None

    return user[0]

def login_user(username, password):
    user = get_user_with_username(username)

    if user == None:
        return False, "Account with this username doesn't exist"

    if user["password"] == password:
        return True, user

    return False, "Incorrect password"

def get_user_chats(user_id):
    user_chats = chats.get_chats_with_user(user_id)

    if user_chats == None:
        return []

    for chat in user_chats:
        first = chat.get("first", None)
        if first == None:
            continue

        if first == user_id:
            chat.pop("first")
            chat["chat_id"] = chat["second"]
            chat.pop("second")
        else:
            chat.pop("second")
            chat["chat_id"] = first
            chat.pop("first")

        partner = get_user(chat["chat_id"])

        if partner != None:
            chat["name"] = partner["username"]

    return user_chats

def get_user_updates(user_id):
    user = get_user(user_id)

    if user == None:
        return None

    updates = user["updates"]

    if len(updates) == 0:
        return updates

    empty_user_updates(user_id)

    return updates

def add_update(user_id, update):
    user = get_user(user_id)

    if user == None:
        return None

    updates = user["updates"]
    update_id = len(updates)

    user["updates"].append(update)

    try:
        lock.acquire(True)

        users.update(user, User.user_id == user_id)
    finally:
        lock.release()

    return True

def empty_user_updates(user_id):
    user = get_user(user_id)

    if user == None:
        return None

    user["updates"] = []

    try:
        lock.acquire(True)

        users.update(user, User.user_id == user_id)
    finally:
        lock.release()

    return True

def search_users(user_id, data):
    all_users = users.all()
    founded = []

    for user in all_users:
        if data in user["username"] and user_id != user["user_id"]:
            founded.append({"user_id":user["user_id"], "username": user["username"]})

    return founded

def count_users():
    try:
        lock.acquire(True)

        all_users = users.all()
    finally:
        lock.release()

    return len(all_users)

def add_user(username, password):
    user = get_user_with_username(username)

    if user != None:
        return False

    user_id = count_users() + 1

    try:
        lock.acquire(True)

        id = users.insert({"username": username, "password": password, "user_id": user_id, "updates":[]})
        user = users.get(doc_id = id)
    finally:
        lock.release()

    return user