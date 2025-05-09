import requests
import xml.etree.ElementTree as ET

params = {
    'apiKey': 'asdfasdfasdf', # API 키는 국토교통부 CCTV API 승인 받은 걸로 넣기
    'type': 'all',
    'cctvType': '1',
    'minX': '126.80',
    'maxX': '127.89',
    'minY': '34.90',
    'maxY': '35.10',
    'getType': 'xml'
}

url = 'https://openapi.its.go.kr:9443/cctvInfo'
response = requests.get(url, params=params)

root = ET.fromstring(response.text)
for data in root.findall('data'):
    name = data.findtext('cctvname')
    cctv_url = data.findtext('cctvurl')
    cctv_format = data.findtext('cctvformat')
    print(f"{name} [{cctv_format}]: {cctv_url}")
