#!/usr/bin/env python3

import yaml
import sys

class ServerNotSatisfyCondition(Exception):
    pass

print("Start generating system-service config file.")
configs = []
filename = sys.argv[1]

try:
    with open(filename, 'r') as file:
        content = file.read()
        content = content[:-1] #delete the last \n output by echo
except FileNotFoundError:
    print(f"File not exist: {filename}")
except IOError:
    print(f"Unable to read file: {filename}")

file_names = content.split(';')
servers = {}
for file_name in file_names:
    try:
        with open(file_name, 'r') as file:
            data = yaml.safe_load(file)
            servers.update(data)
    except FileNotFoundError:
        print(f"File not exist: {file_name}")
    except IOError:
        print(f"Unable to read file: {file_name}")
        
servers_iter = iter(servers.items())

for k,v in servers_iter:
    try:
        if not k in sys.argv:
            raise ServerNotSatisfyCondition
        server_template:dict = v
        if server_template['conditions'] != None:
            conditions:list = server_template['conditions']
            for condition_group in conditions:
                valid = False
                for condition_name in condition_group:
                    if condition_name in sys.argv:
                           valid = True
                if not valid:
                    raise ServerNotSatisfyCondition
                    
    except ServerNotSatisfyCondition:
        continue
    server = {}
    server["filename"] = server_template["filename"]
    server["boot_time"] = server_template["boot_time"]
    server["registration_method"] = server_template["registration_method"]
    server["type"] = server_template["type"]
    server["services"] = []
    service_list:list = server_template["services"]
    service_iter = iter(service_list)
    for service in service_iter:
        try:
            service:dict
            service_conditions = service["conditions"]
            if not service["conditions"] == None:
                for service_condition_group in service_conditions:
                    valid = False
                    for service_condition_name in service_condition_group:
                        if service_condition_name in sys.argv:
                            valid = True
                    if not valid:
                        raise ServerNotSatisfyCondition
            server["services"].append(service["name"])
        except ServerNotSatisfyCondition:
            continue
    configs.append(server)
    
    
user_servers = []
if sys.argv[2]:
    try:
        with open(sys.argv[2], 'r') as file:
            content = file.read()
            content = content[:-1] # delete the last \n output by echo
            file_names = content.split(';')
            if len(file_names) == 1 and file_names[0] == "":
                file_names = []
            for file_name in file_names:
                try:
                    with open(file_name, 'r') as file:
                        data = yaml.safe_load(file)
                        user_servers.append(data)
                except FileNotFoundError:
                    print(f"File not exist: {file_name}")
                except IOError:
                    print(f"Unable to read file: {file_name}")
    except FileNotFoundError:
        print(f"File not exist: {sys.argv[2]}")
    except IOError:
        print(f"Unable to read file: {sys.argv[2]}")
configs.extend(user_servers)
yaml.dump(configs, open(sys.argv[3], "w"))
print("Complete generating system-service config file.")