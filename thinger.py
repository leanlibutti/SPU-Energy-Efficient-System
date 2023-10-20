import json
import urllib.request as urlreq


thinger_config = json.load(open("thinger_config.json"))
user = thinger_config["user"]
adminToken = thinger_config["admin_token"]


def createBucket(id, name, description):
	data = { "active": True, "enabled": True, "bucket": id, "name":name, "description": description, "source": "write"}

	jsondata = json.dumps(data).encode("utf-8")
	req = urlreq.Request("http://0.0.0.0/v1/users/{0}/buckets?".format(user));
	req.add_header("Content-Type", "application/json;charset=UTF-8");
	req.add_header("Authorization", "Bearer {0}".format(adminToken));
	req.add_header("Content-Length", len(jsondata))
	response = urlreq.urlopen(req, jsondata);


def createRoomDashboard(id, name, description):
	createJson = json.load(open("json/createDashboard.json"))

	createJson["dashboard"] = id;
	createJson["name"] = name;
	createJson["description"] = description;

	jsondata = json.dumps(createJson).encode("utf-8")
	req = urlreq.Request("http://0.0.0.0/v1/users/{0}/dashboards?".format(user));
	req.add_header("Content-Type", "application/json;charset=UTF-8");
	req.add_header("Authorization", "Bearer {0}".format(adminToken));
	req.add_header("Content-Length", len(jsondata))
	response = urlreq.urlopen(req, jsondata);

	#if response.getcode() != 200:
	#	print("Codigo de creacion: {0}".format(response.getcode()))
	#	return

	shareJson = json.load(open("json/shareDashboard.json"))

	shareJson["token"] = "Dashboard_"+id;
	shareJson["name"] = "Token para dashboard "+name;
	shareJson["allow"]["Dashboard"] = {
		id: {"action":["ReadDashboardConfig"]}
	}

	#print(json.dumps(shareJson));
	jsondata = json.dumps(shareJson).encode("utf-8")
	req = urlreq.Request("http://0.0.0.0/v1/users/{0}/tokens?".format(user));
	req.add_header("Content-Type", "application/json;charset=UTF-8");
	req.add_header("Authorization", "Bearer {0}".format(adminToken));
	req.add_header("Content-Length", len(jsondata))
	response = urlreq.urlopen(req, jsondata);

	#if response.getcode() != 200:
	#	print("Codigo de compartir: {0}".format(response.getcode()))
	#	return
	
	widgetJson = json.load(open("json/addWidgets.json"))
	jsonw = json.load(open("json/widget.json"))

	widgetJson["dashboard"] = id;
	widgetJson["description"] = description;
	widgetJson["name"] = name;
	widgetJson["widgets"] = [jsonw]

	#print(json.dumps(shareJson));
	jsondata = json.dumps(widgetJson).encode("utf-8")
	req = urlreq.Request("http://0.0.0.0/v1/users/{0}/dashboards/{1}?".format(user, id), method= 'PUT');
	req.add_header("Content-Type", "application/json;charset=UTF-8");
	req.add_header("Authorization", "Bearer {0}".format(adminToken));
	req.add_header("Content-Length", len(jsondata))
	response = urlreq.urlopen(req, jsondata);


def createDevice(id, description):
	deviceJson = {
		"device_id": str(id),
		"device_description": str(description),
		"device_credentials": "123456"
	}
	jsondata = json.dumps(deviceJson).encode("utf-8")
	req = urlreq.Request("http://0.0.0.0/v1/users/{0}/devices".format(user, id));
	req.add_header("Content-Type", "application/json;charset=UTF-8");
	req.add_header("Authorization", "Bearer {0}".format(adminToken));
	req.add_header("Content-Length", len(jsondata))
	response = urlreq.urlopen(req, jsondata);