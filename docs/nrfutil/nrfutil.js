// 書き込み用ラッピングライブラリ

var nrfutil = {};

// 指定したURLのファイルをダウンロードする
nrfutil.ajax_array_buffer = function(src) {
    return new Promise((resolve, reject) => {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', src, true);
        xhr.responseType = "arraybuffer"; // arraybuffer blob text json 
        xhr.onload = function(e) {
            if (xhr.status == 200) {
                resolve(xhr.response);
            } else {
                resolve(null);
            }
        };
        xhr.send();
    });
};

nrfutil.write = async function(zip_uri) {
    // シリアルポートオブジェクト用意
    var transport = new DfuTransportSerial({
        baudRate: 115200,
        flowControl: false,
        singleBank: false
    });

    // シリアルポート接続
    // シリアルポート選択のウィンドウが出て選択する
    // (このメソッドはバックエンドで動き続けないといけないので非同期で実行)
    transport.open();

    // 接続されるまで待つ
    while (!transport.isOpen()) {
        await new Promise(resolve => setTimeout(resolve, 1000));
    }

    // ZIPファイルをダウンロード
    var firmwareData = await nrfutil.ajax_array_buffer(zip_uri);
    if (!firmwareData) { // ダウンロード失敗
        transport.logger.error("download error: " + zip_uri); // メッセージ表示
        transport.close(); // シリアルポート閉じる
        return;
    }

    // DFUオブジェクト用意
    var dfuInstance = new Dfu(firmwareData, transport);
    
    // ZIP 解凍
    await dfuInstance.initialize();
    
    // ファームウェア送信
    await dfuInstance.executeDfu();
    
    // シリアルポートクローズ
    transport.close();
};
