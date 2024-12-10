import 'dart:async';
import 'dart:math';
import 'package:intl/intl.dart'; // 引入 intl 包用于格式化时间
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import "../utils/snackbar.dart";

import "descriptor_tile.dart";

import 'package:shared_preferences/shared_preferences.dart'; // Used to save data

class CharacteristicTile extends StatefulWidget {
  final BluetoothCharacteristic characteristic;
  final List<DescriptorTile> descriptorTiles;

  const CharacteristicTile({Key? key, required this.characteristic, required this.descriptorTiles}) : super(key: key);

  @override
  State<CharacteristicTile> createState() => _CharacteristicTileState();
}

class _CharacteristicTileState extends State<CharacteristicTile> {
  List<int> _value = [];
  Timer? _readTimer; // 定时器变量

  late StreamSubscription<List<int>> _lastValueSubscription;

  @override
  void initState() {
    super.initState();

    // 开始监听特性值的变化
    _lastValueSubscription = widget.characteristic.lastValueStream.listen((value) {
      _value = value;
      if (mounted) {
        setState(() {});
      }
    });

    // 每隔 5 分钟调用一次 onReadPressed
    _readTimer = Timer.periodic(Duration(minutes: 5), (timer) async {
      if (mounted) {
        await onReadPressed();
      }
    });
  }

  @override
  void dispose() {
    _lastValueSubscription.cancel(); // 取消监听
    _readTimer?.cancel(); // 清理定时器
    super.dispose();
  }

  BluetoothCharacteristic get c => widget.characteristic;

  // 从字符串中提取温度和湿度
  Map<String, double> extractTemperatureAndHumidity(String data) {
    try {
      // 使用正则表达式提取温度和湿度值
      final tempMatch = RegExp(r"Temp:\s*([\d.]+)C").firstMatch(data);
      final humidityMatch = RegExp(r"Humidity:\s*([\d.]+)%").firstMatch(data);

      // 将提取到的值转换为 double
      final temp = tempMatch != null ? double.parse(tempMatch.group(1)!) : 0.0;
      final humidity = humidityMatch != null ? double.parse(humidityMatch.group(1)!) : 0.0;

      return {"temp": temp, "humidity": humidity};
    } catch (e) {
      return {"temp": 0.0, "humidity": 0.0}; // 如果解析失败，返回默认值
    }
  }

  Future onReadPressed() async {
    try {
      await c.read(); // 从蓝牙读取数据
      Snackbar.show(ABC.c, "Read: Success", success: true);

      // 转换字节数据为字符串
      String decodedData = String.fromCharCodes(_value);

      // 从字符串中提取温度和湿度
      final extractedData = extractTemperatureAndHumidity(decodedData);
      double temp = extractedData["temp"]!;
      double humidity = extractedData["humidity"]!;

      // 保存温度和湿度数据
      await saveData(temp.toStringAsFixed(1), humidity.toStringAsFixed(1));

      Snackbar.show(
        ABC.c,
        "Data saved: Temp: ${temp.toStringAsFixed(1)}C, Humidity: ${humidity.toStringAsFixed(1)}%",
        success: true,
      );

      if (mounted) {
        setState(() {});
      }
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Read Error:", e), success: false);
    }
  }

  Future onWritePressed() async {
    try {
      await c.write(_getRandomBytes(), withoutResponse: c.properties.writeWithoutResponse);
      Snackbar.show(ABC.c, "Write: Success", success: true);
      if (c.properties.read) {
        await c.read();
      }
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Write Error:", e), success: false);
    }
  }

  Future onSubscribePressed() async {
    try {
      await c.setNotifyValue(!c.isNotifying); // Toggle the notifying state
      Snackbar.show(ABC.c, "Subscribe successfully", success: true); // Fixed success message
      if (c.properties.read) {
        await c.read(); // Optionally read the characteristic value after subscribing
      }
      if (mounted) {
        setState(() {});
      }
    } catch (e) {
      Snackbar.show(ABC.c, "Subscribe successfully", success: true); // Show error message
    }
  }

  Widget buildUuid(BuildContext context) {
    String uuid = '0x${widget.characteristic.uuid.str.toUpperCase()}';
    return Text(uuid, style: const TextStyle(fontSize: 13));
  }

  Widget buildValue(BuildContext context) {
    // 转换字节数据为字符串
    String data = String.fromCharCodes(_value);

    // 定义正则表达式匹配各个字段
    final tempMatch = RegExp(r"Temp:\s*([\d.]+)C").firstMatch(data);
    final humidityMatch = RegExp(r"Humidity:\s*([\d.]+)%").firstMatch(data);
    final rxBattMatch = RegExp(r"RX Batt:\s*(.+?)%").firstMatch(data);
    final txBattMatch = RegExp(r"TX Batt:\s*(.+?)%").firstMatch(data);
    final rssiMatch = RegExp(r"Signal Quality:\s*(.*)").firstMatch(data);

    // 提取字段值，找不到时显示默认值
    final temp = tempMatch != null ? tempMatch.group(1) : "--";
    final humidity = humidityMatch != null ? humidityMatch.group(1) : "--";
    final rxBatt = rxBattMatch != null ? rxBattMatch.group(1) : "--";
    final txBatt = txBattMatch != null ? txBattMatch.group(1) : "--";
    final rssi = rssiMatch != null ? rssiMatch.group(1) : "--";

  // 构建优化后的 UI 显示
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text("Temp: $temp°C",
            style: const TextStyle(fontSize: 32, color: Colors.red),
            textAlign: TextAlign.left),
        Text("Humidity: $humidity%",
            style: const TextStyle(fontSize: 32, color: Colors.red),
            textAlign: TextAlign.left),
        Text("Receiver Battary: $rxBatt%",
            style: const TextStyle(fontSize: 32, color: Colors.red),
            textAlign: TextAlign.left),
        Text("Transmitter Battary: $txBatt%",
            style: const TextStyle(fontSize: 32, color: Colors.red),
            textAlign: TextAlign.left),
        Text("Signal Quality: $rssi",
           style: const TextStyle(fontSize: 32, color: Colors.red),
            textAlign: TextAlign.left),
      ],
    );
  }


  Widget buildLargeButton(BuildContext context, String label, VoidCallback onPressed) {
    return ElevatedButton(
      onPressed: onPressed,
      child: Text(label),
      style: ElevatedButton.styleFrom(
        padding: const EdgeInsets.symmetric(vertical: 12.0, horizontal: 24.0),
        textStyle: const TextStyle(fontSize: 16),
      ),
    );
  }

  Widget buildButtonRow(BuildContext context) {
    bool read = widget.characteristic.properties.read;
    bool write = widget.characteristic.properties.write;
    bool notify = widget.characteristic.properties.notify;
    bool indicate = widget.characteristic.properties.indicate;

    return Column(
      children: [
        if (read)
          buildLargeButton(
            context,
            "Read",
            () async {
              await onReadPressed();
              if (mounted) setState(() {});
            },
          ),
        if (write)
          buildLargeButton(
            context,
            widget.characteristic.properties.writeWithoutResponse ? "WriteNoResp" : "Write",
            () async {
              await onWritePressed();
              if (mounted) setState(() {});
            },
          ),
        if (notify || indicate)
          buildLargeButton(
            context,
            widget.characteristic.isNotifying ? "Unsubscribe" : "Subscribe",
            () async {
              await onSubscribePressed();
              if (mounted) setState(() {});
            },
          ),
        buildLargeButton(
          context,
          "View Saved Data",
          () => showSavedData(context),
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return ExpansionTile(
      title: ListTile(
        title: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: <Widget>[
            const Text('Characteristic'),
            buildUuid(context),
            buildValue(context),
          ],
        ),
        subtitle: buildButtonRow(context),
        contentPadding: const EdgeInsets.all(0.0),
      ),
      children: widget.descriptorTiles,
    );
  }

  // Used to view saved data
  Future<void> showSavedData(BuildContext context) async {
    final prefs = await SharedPreferences.getInstance();
    List<String>? storedData = prefs.getStringList('bluetoothData') ?? [];

    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text("Saved Data"),
          content: SingleChildScrollView(
            child: Text(storedData.join('\n')), // Display saved data
          ),
          actions: [
            TextButton(
              child: const Text("Clear Data"),
              onPressed: () async {
                // Clear data in SharedPreferences
                await prefs.remove('bluetoothData');
                Navigator.of(context).pop(); // Close the dialog
                Snackbar.show(context as ABC, "All data cleared!", success: true); // Show confirmation
              },
            ),
            TextButton(
              child: const Text("OK"),
              onPressed: () {
                Navigator.of(context).pop(); // Close the dialog
              },
            ),
          ],
        );
      },
    );
  }

  // Generate random bytes for write operations
  List<int> _getRandomBytes() {
    final math = Random();
    return [math.nextInt(255), math.nextInt(255), math.nextInt(255), math.nextInt(255)];
  }
}

// Function to save data
Future<void> saveData(String temp, String humidity) async {
  final prefs = await SharedPreferences.getInstance();
  List<String>? storedData = prefs.getStringList('bluetoothData') ?? [];

  // 创建新的数据条目
  String timestamp = DateFormat('yyyy-MM-dd HH:mm:ss').format(DateTime.now());
  Map<String, String> newData = {
    "temp": temp,
    "humidity": humidity,
    "time": timestamp,
  };

  // 添加新数据
  storedData.add(newData.toString());
  if (storedData.length > 1000) {
    storedData = storedData.sublist(storedData.length - 1000); // 仅保留最近 1000 条数据
  }

  await prefs.setStringList('bluetoothData', storedData); // 保存更新后的数据
}
