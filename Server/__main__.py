import threading
import users
import chats
import jwt
from crypt import get_secret_key

from functools import wraps
from responses import SuccessResponse, ErrorResponse

from flask import Flask, request

app = Flask(__name__)

def login_required(f):
    @wraps(f)
    def wrap(*args, **kwargs):
        authorization = request.headers.get("Authorization", None, type=str)
        if authorization == None:
            return ErrorResponse("Authorization header not provided", 400)
        
        token = authorization[7:]
        try:
            token_info = jwt.decode(token, get_secret_key(), algorithms='HS256')
            user_id = token_info.get("user_id", None)
        except Exception as e:
            return ErrorResponse("Invalid authorization token", 400)

        return f(user_id, *args, **kwargs)

    return wrap

# https://127.0.0.1:443/api/v1/sendMessage?chat_id=2&text=12345
@app.route('/api/v1/sendMessage')
@login_required
def send_message(user_id:int):
    arguments = request.args

    text = arguments.get("text")
    chat_id = arguments.get("chat_id", type=int)

    if chat_id == None:
        return ErrorResponse("Invalid chat_id", 400)

    if text == None:
        return ErrorResponse("text equals nil", 400)
    elif text.__len__() == 0:
        return ErrorResponse("text length equals 0", 400)
    elif text.__len__() > 4960:
        return ErrorResponse("text length is too much", 400)

    if users.get_user(user_id) == None:
        return ErrorResponse("Ivalid user_id", 400)

    if chat_id > 0:
        if users.get_user(chat_id) == None:
            return ErrorResponse("Ivalid chat_id", 400)

        chat = chats.get_chat_with_users(user_id, chat_id)
        
        if chat == None:
            chat = chats.add_chat(user_id, chat_id)

        if chat == None:
            return ErrorResponse("User or chat_id not exists", 400)
        
        message = chats.add_message_to_chat(chat["users"][0], chat["users"][1], {"from": user_id, "text":text})
        
        return SuccessResponse(message)
    elif chat_id < 0:
        chat = chats.get_group_with_id(chat_id)
        
        if chat == None:
            return ErrorResponse("Invalid chat_id", 400)
        
        message = chats.add_message_to_group(user_id, chat_id, {"from": user_id, "text":text})
        
        return SuccessResponse(message)
    else:
        return ErrorResponse("This chat_id not supported", 500)
    
# https://127.0.0.1:443/api/v1/createGroup
@app.route('/api/v1/createGroup', methods=["POST"])
@login_required
def create_group(user_id:int):
    
    body = request.get_json(silent=True)

    group_name = body.get("name", None)
    group_users = body.get("users", None)

    if type(group_users) != list:
        return ErrorResponse("users are incorrect", 400)

    if group_name == None:
        return ErrorResponse("group name equals nil", 400)
    elif type(group_name) != str:
        return ErrorResponse("group name must be string", 400)
    elif group_name.__len__() == 0:
        return ErrorResponse("group name length equals 0", 400)
    elif group_name.__len__() > 32:
        return ErrorResponse("group name length is too much", 400)

    if users.get_user(user_id) == None:
        return ErrorResponse("Ivalid user_id", 400)

    group = chats.create_group(group_name, user_id, group_users)
    
    return SuccessResponse(group)

# https://127.0.0.1:443/api/v1/getUpdates
@app.route('/api/v1/getUpdates')
@login_required
def get_updates(user_id:int):
    notifier = threading.Event()

    chats.notify.append([user_id, notifier])

    updates = users.get_user_updates(user_id)

    if len(updates) == 0:
        notifier.wait(20)

        updates = users.get_user_updates(user_id)

        for i in chats.notify:
            if i[1] == notifier:
                chats.notify.remove(i)
    
    return SuccessResponse(updates)

# https://127.0.0.1:443/api/v1/getChats
@app.route('/api/v1/getChats')
@login_required
def get_chats(user_id:int):
    user = users.get_user(user_id)

    if user != None:
        users.empty_user_updates(user_id)
        
        user_chats = users.get_user_chats(user_id)

        print(user_chats)
        
        return SuccessResponse({"chats": user_chats})

    return ErrorResponse("User with this id doesn't exist", 400)

# https://127.0.0.1:443/api/v1/search?data=KEK
@app.route('/api/v1/search')
@login_required
def search(user_id:int):
    arguments = request.args

    data = arguments.get("data")

    if data == None:
        return ErrorResponse("data equals None", 400)
    elif data.__len__() == 0:
        return ErrorResponse("data length cannot be 0", 400)
    elif data.__len__() > 32:
        return ErrorResponse("data length must be less or equal 32", 400)

    founded_users = users.search_users(data)

    return SuccessResponse({"users": founded_users})

# https://127.0.0.1:443/api/v1/createAccount?username=HAH&password=12345
@app.route('/api/v1/createAccount', methods=["POST"])
def create_account():
    body = request.get_json(silent=True)

    username = body.get("username")
    password = body.get("password")

    if username == None:
        return ErrorResponse("Username equals nil", 400)
    elif username.__len__() == 0:
        return ErrorResponse("Username length cannot be 0", 400)
    elif username.__len__() > 32:
        return ErrorResponse("Username length must be less or equal 32", 400)

    if password == None:
        return ErrorResponse("Password equals nil", 400)
    elif password.__len__() == 0:
        return ErrorResponse("Password length cannot be 0", 400)
    elif password.__len__() > 32:
        return ErrorResponse("Password length must be less or equal 32", 400)

    user = users.add_user(username, password)
    if user == False:
        return ErrorResponse("User with this username already exist", 400)

    return SuccessResponse(user)

# https://127.0.0.1:443/api/v1/login?username=HAH&password=12345
@app.route('/api/v1/login', methods=["POST"])
def login():
    body = request.get_json()

    username = body.get("username")
    password = body.get("password")

    print(username)
    print(password)
    print(body)

    if username == None:
        return ErrorResponse("username equals nil", 400)
    elif username.__len__() == 0:
        return ErrorResponse("username length cannot be 0", 400)
    elif username.__len__() > 32:
        return ErrorResponse("username length must be less or equal 32", 400)

    if password == None:
        return ErrorResponse("password equals nil", 400)
    elif password.__len__() == 0:
        return ErrorResponse("password length cannot be 0", 400)
    elif password.__len__() > 32:
        return ErrorResponse("password length must be less or equal 32", 400)

    success, user = users.login_user(username, password)
    if success == False:
        return ErrorResponse(user, 400)

    return SuccessResponse(user)

@app.route('/')
def index():
    return "Hello, World!"

if __name__ == '__main__':
    context = ("server.pem",)
    app.run(debug=True, ssl_context=context, port=443, host="0.0.0.0")
