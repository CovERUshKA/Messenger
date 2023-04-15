from flask import jsonify

def SuccessResponse(result):
    response = {"ok":True, "result":result}

    return jsonify(response)

#{
#   ok: false,
#   error_code: error_code,
#   description: "wewewe"
#}
def ErrorResponse(description, error_code):
    response = {"ok":False, "error_code":error_code, "description":description}

    return jsonify(response), error_code
