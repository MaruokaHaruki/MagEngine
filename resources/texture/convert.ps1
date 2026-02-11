# ミップレベルを指定(0=自動生成)
$mipLevel = 3

# jpg と png のファイルを取得
$files = Get-Item *.jpg, *.png

# 取得したファイルを順番に処理
foreach($f in $files){
    # ミップレベルオプション付きで実行
    Start-Process -FilePath TextureConverter.exe -ArgumentList $f, "-ml", $mipLevel -Wait
}

pause