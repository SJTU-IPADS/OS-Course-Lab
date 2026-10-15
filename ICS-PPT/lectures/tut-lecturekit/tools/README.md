# tools

`shot.js` 重新截取教程里的网页截图（下载页、GitHub 页面），`jobs.json` 列出每张图的网址、红框位置和用户代理。版本号变化后在 `ICS-PPT` 根目录运行（需要先 `scripts/prepare.sh`，并装有 Google Chrome）：

```bash
node lectures/tut-lecturekit/tools/shot.js lectures/tut-lecturekit/tools/jobs.json
```

课件界面的几张图（`viewer-*.png`）取自 `render lectures/1-intro` 生成的 `build/ics-intro-viewer/index.html`、`slides.html#2`、`slides.html#5`，以及一次 `view lectures/1-intro --watch --port 3031` 会话中第 37 页按下 ▶ 之后的画面，视口 1280×800，去掉上下的黑边。
