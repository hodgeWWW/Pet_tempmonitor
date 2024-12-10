import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'device_screen.dart';
import '../utils/snackbar.dart';

class ScanScreen extends StatefulWidget {
  const ScanScreen({Key? key}) : super(key: key);

  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  BluetoothDevice? _heatHoundDevice;
  bool _isScanning = false;
  String _statusMessage = "Searching for 'Heat Hound'...";

  late StreamSubscription<List<ScanResult>> _scanResultsSubscription;
  late StreamSubscription<bool> _isScanningSubscription;

  @override
  void initState() {
    super.initState();

    // Listen to scan results and filter for "Heat Hound" device
    _scanResultsSubscription = FlutterBluePlus.scanResults.listen((results) {
      final heatHoundDevices = results.where((result) => result.device.name == "Heat Hound").toList();

      if (heatHoundDevices.isNotEmpty) {
        _heatHoundDevice = heatHoundDevices.first.device;
        _statusMessage = "'Heat Hound' is available to connect.";
      } else {
        _heatHoundDevice = null;
        _statusMessage = "'Heat Hound' not found. Please try scanning again.";
      }

      if (mounted) {
        setState(() {});
      }
    }, onError: (e) {
      Snackbar.show(ABC.b, prettyException("Scan Error:", e), success: false);
    });

    // Listen to scanning state
    _isScanningSubscription = FlutterBluePlus.isScanning.listen((state) {
      _isScanning = state;
      if (mounted) {
        setState(() {});
      }
    });
  }

  @override
  void dispose() {
    _scanResultsSubscription.cancel();
    _isScanningSubscription.cancel();
    super.dispose();
  }

  Future onScanPressed() async {
    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
      setState(() {
        _statusMessage = "Searching for 'Heat Hound'...";
      });
    } catch (e) {
      Snackbar.show(ABC.b, prettyException("Start Scan Error:", e), success: false);
    }
  }

  Future onStopPressed() async {
    try {
      FlutterBluePlus.stopScan();
    } catch (e) {
      Snackbar.show(ABC.b, prettyException("Stop Scan Error:", e), success: false);
    }
  }

  void onConnectPressed() {
    if (_heatHoundDevice != null) {
      _heatHoundDevice!.connect().catchError((e) {
        Snackbar.show(ABC.c, prettyException("Connect Error:", e), success: false);
      });
      MaterialPageRoute route = MaterialPageRoute(
          builder: (context) => DeviceScreen(device: _heatHoundDevice!),
          settings: RouteSettings(name: '/DeviceScreen'));
      Navigator.of(context).push(route);
    }
  }

  Widget buildScanButton(BuildContext context) {
    return ElevatedButton(
      onPressed: _isScanning ? onStopPressed : onScanPressed,
      style: ElevatedButton.styleFrom(
        padding: EdgeInsets.symmetric(horizontal: 24, vertical: 16), // Larger button size
        backgroundColor: Colors.red, // Use backgroundColor instead of primary
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(30),
        ),
      ),
      child: Text(_isScanning ? "STOP" : "SCAN", style: TextStyle(fontSize: 24)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return ScaffoldMessenger(
      key: Snackbar.snackBarKeyB,
      child: Scaffold(
        appBar: AppBar(
          title: const Text('Find Devices'),
        ),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              // Show status message or connection button
              Text(
                _statusMessage,
                style: TextStyle(fontSize: 18, color: Colors.black54),
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 20),
              if (_heatHoundDevice != null)
                ElevatedButton(
                  onPressed: onConnectPressed,
                  child: Text("Connect to Heat Hound", style: TextStyle(fontSize: 20)),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.green),
                ),
            ],
          ),
        ),
        floatingActionButtonLocation: FloatingActionButtonLocation.centerFloat,
        floatingActionButton: buildScanButton(context),
      ),
    );
  }
}
