import threading
import users
import chats

from flask import Flask, jsonify, request

app = Flask(__name__)

def SuccessResponse(result):
    response = {"ok":True, "result":result}

    return jsonify(response)

#{
#   ok: false,
#   error_code: error_code,
#   description: "wewewe"
# }
def ErrorResponse(description, error_code):
    response = {"ok":False, "error_code":error_code, "description":description}

    return jsonify(response), error_code

# https://127.0.0.1:443/api/v1/sendMessage?user_id=1&chat_id=2&text=12345
@app.route('/api/v1/sendMessage')
def send_message():
    arguments = request.args

    text = arguments.get("text")
    str_user_id = arguments.get("user_id")
    str_chat_id = arguments.get("chat_id")

    if str_user_id == None:
        return ErrorResponse("user_id equals nil", 400)
    elif not str_user_id.isdecimal():
        return ErrorResponse("user_id not is decimal", 400)

    if str_chat_id == None:
        return ErrorResponse("chat_id equals nil", 400)
    elif not str_chat_id.isdecimal():
        return ErrorResponse("chat_id not is decimal", 400)

    if text == None:
        return ErrorResponse("text equals nil", 400)
    elif text.__len__() == 0:
        return ErrorResponse("text length equals 0", 400)
    elif text.__len__() > 4960:
        return ErrorResponse("text length is too much", 400)

    user_id = int(str_user_id)
    chat_id = int(str_chat_id)

    if users.get_user(user_id) == None:
        return ErrorResponse("Ivalid user_id", 400)

    if chat_id > 0:
        if users.get_user(chat_id) == None:
            return ErrorResponse("Ivalid chat_id", 400)

        chat = chats.get_chat_with_users(user_id, chat_id)
        
        if chat == None:
            chat = chats.add_chat(user_id, chat_id)
            
        print(chat)

        if chat == None:
            return ErrorResponse("User or chat_id not exists", 400)
        
        message = chats.add_message_to_chat(chat["first"], chat["second"], {"from": user_id, "text":text})
        
        return SuccessResponse(message)
    else:
        return ErrorResponse("This chat_id not supported", 500)

# https://127.0.0.1:443/api/v1/getUpdates?user_id=1
@app.route('/api/v1/getUpdates')
def get_updates():
    arguments = request.args

    str_user_id = arguments.get("user_id")

    if str_user_id == None:
        return ErrorResponse("user_id equals nil", 400)
    elif not str_user_id.isdecimal():
        return ErrorResponse("user_id not is decimal", 400)

    user_id = int(str_user_id)

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

# https://127.0.0.1:443/api/v1/getChats?user_id=1
@app.route('/api/v1/getChats')
def get_chats():
    arguments = request.args

    str_user_id = arguments.get("user_id")

    if str_user_id == None:
        return ErrorResponse("user_id equals nil", 400)
    elif not str_user_id.isdecimal():
        return ErrorResponse("user_id not is decimal", 400)

    user_id = int(str_user_id)

    user = users.get_user(user_id)

    if user != None:
        users.empty_user_updates(user_id)
        
        user_chats = users.get_user_chats(user_id)
        
        return SuccessResponse({"chats": user_chats})

    return ErrorResponse("User with this id doesn't exist", 400)

# https://127.0.0.1:443/api/v1/search?user_id=1&data=KEK
@app.route('/api/v1/search')
def search():
    arguments = request.args

    data = arguments.get("data")
    str_user_id = arguments.get("user_id")

    if str_user_id == None:
        return ErrorResponse("user_id equals nil", 400)
    elif not str_user_id.isdecimal():
        return ErrorResponse("user_id not is decimal", 400)

    if data == None:
        return ErrorResponse("data equals None", 400)
    elif data.__len__() == 0:
        return ErrorResponse("data length cannot be 0", 400)
    elif data.__len__() > 32:
        return ErrorResponse("data length must be less or equal 32", 400)

    user_id = int(str_user_id)

    founded_users = users.search_users(user_id, data)

    return SuccessResponse({"users": founded_users})

# https://127.0.0.1:443/api/v1/createAccount?username=HAH&password=12345
@app.route('/api/v1/createAccount')
def create_account():
    arguments = request.args

    username = arguments.get("username")
    password = arguments.get("password")

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

    #return "success"

    if users.get_user_with_username(username):
        return ErrorResponse("User with this username already exists", 400)

    user = users.add_user(username, password)
    if user == False:
        return ErrorResponse("User with this username already exist", 400)

    return SuccessResponse(user)

# https://127.0.0.1:443/api/v1/login?username=HAH&password=12345
@app.route('/api/v1/login')
def login():
    arguments = request.args

    username = arguments.get("username")
    password = arguments.get("password")

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

    #return "success"

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