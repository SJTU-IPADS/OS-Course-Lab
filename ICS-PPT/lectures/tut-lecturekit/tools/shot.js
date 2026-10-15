// Screenshot helper for the tutorial's images: node tools/shot.js tools/jobs.json
// Run from the ICS-PPT (lecturekit) root after scripts/prepare.sh: puppeteer-core
// comes with the vendored marp-cli, and the local Google Chrome drives the pages.
// Each job: {name, url, ua?, width?, height?, scale?, waits?: ms, scrollY?,
//            clicks?: [selector], clickText?: [[tag, text]],
//            highlight?: [selector], highlightText?: [[tag, text, closestSelector?]],
//            evalHighlight?: "js returning elements", boxAround?: "js returning elements",
//            afterClick?: "js run after the clicks" (the mouse is moved to the corner first),
//            scrollTo?: selector, css?: string, fullPage?: bool, out?: dir}
// Output: images/<name>.png next to this script.
const path = require('path');
const puppeteer = require(path.join(process.cwd(), 'node_modules', 'puppeteer-core'));
const fs = require('fs');
const OUT = path.join(__dirname, '..', 'images');
const CHROME = process.env.CHROME_PATH || '/usr/bin/google-chrome';
const UA = {
  win: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36',
  mac: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36',
  linux: 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36',
};
(async () => {
  const jobs = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
  const browser = await puppeteer.launch({
    executablePath: CHROME,
    headless: true,
    args: ['--no-sandbox', '--lang=en-US', '--hide-scrollbars'],
  });
  for (const job of jobs) {
    const page = await browser.newPage();
    if (job.css) page.on('load', () => {});
    try {
      await page.setUserAgent(UA[job.ua || 'win']);
      await page.setViewport({ width: job.width || 1280, height: job.height || 800, deviceScaleFactor: job.scale || 1 });
      await page.goto(job.url, { waitUntil: "networkidle2", timeout: 90000 }); if (job.css) await page.addStyleTag({ content: job.css });
      await new Promise(r => setTimeout(r, job.waits || 1500));
      for (const sel of job.clicks || []) {
        await page.waitForSelector(sel, { timeout: 15000 });
        await page.click(sel);
        await new Promise(r => setTimeout(r, 1200));
      }
      for (const [tag, text] of job.clickText || []) {
        const ok = await page.evaluate((tag, text) => {
          const el = [...document.querySelectorAll(tag)].find(e => (e.innerText || '').trim() === text);
          if (!el) return false; el.click(); return true;
        }, tag, text);
        console.log(`[${job.name}] clickText ${tag} "${text}" -> ${ok}`);
        await new Promise(r => setTimeout(r, 1500));
      }
      if (job.afterClick) {
        await page.mouse.move(5, 5);
        await page.evaluate(code => eval(code), job.afterClick);
        await new Promise(r => setTimeout(r, 500));
      }
      if (job.dump) {
        const items = await page.$$eval(job.dump, els => els.slice(0, 80).map(e => (e.innerText || '').trim().replace(/\s+/g, ' ').slice(0, 120) + '  ->  ' + (e.getAttribute('href') || e.getAttribute('aria-label') || '')));
        console.log(`[${job.name}] dump of ${job.dump}:`); items.forEach((t, i) => console.log('  ', i, t));
      }
      if (job.scrollTo) {
        await page.evaluate(sel => { const e = document.querySelector(sel); if (e) e.scrollIntoView({ block: 'center' }); }, job.scrollTo);
        await new Promise(r => setTimeout(r, 600));
      }
      if (job.scrollY) { await page.evaluate(y => window.scrollTo(0, y), job.scrollY); await new Promise(r => setTimeout(r, 500)); }
      for (const sel of job.highlight || []) {
        const n = await page.evaluate(sel => {
          const els = document.querySelectorAll(sel);
          els.forEach(e => { e.style.outline = '4px solid #e53935'; e.style.outlineOffset = '4px'; e.style.borderRadius = '4px'; });
          return els.length;
        }, sel);
        console.log(`[${job.name}] highlighted ${n} x ${sel}`);
      }
      for (const [tag, text, closest] of job.highlightText || []) {
        const n = await page.evaluate((tag, text, closest) => {
          let els = [...document.querySelectorAll(tag)].filter(e => (e.innerText || '').trim() === text);
          if (closest) els = els.map(e => e.closest(closest)).filter(Boolean);
          els.forEach(e => { e.style.outline = '4px solid #e53935'; e.style.outlineOffset = '4px'; e.style.borderRadius = '4px'; });
          if (els[0]) els[0].scrollIntoView({ block: 'center' });
          return els.length;
        }, tag, text, closest || null);
        console.log(`[${job.name}] highlightText ${tag} "${text}" -> ${n}`);
        await new Promise(r => setTimeout(r, 500));
      }
      if (job.evalHighlight) {
        const n = await page.evaluate(code => {
          const els = eval(code);
          els.forEach(e => { e.style.outline = '4px solid #e53935'; e.style.outlineOffset = '4px'; e.style.borderRadius = '4px'; });
          return els.length;
        }, job.evalHighlight);
        console.log(`[${job.name}] evalHighlight -> ${n}`);
      }
      if (job.boxAround) {
        const n = await page.evaluate(code => {
          const els = eval(code).filter(e => { const q=e.getBoundingClientRect(); return q.width>0 && q.height>0; }); if (!els.length) return 0;
          let l=1e9,t=1e9,r=-1e9,b=-1e9;
          els.forEach(e => { const q=e.getBoundingClientRect(); l=Math.min(l,q.left); t=Math.min(t,q.top); r=Math.max(r,q.right); b=Math.max(b,q.bottom); });
          const d=document.createElement('div'); d.style.cssText=`position:fixed;left:${l-6}px;top:${t-6}px;width:${r-l+12}px;height:${b-t+12}px;border:4px solid #e53935;border-radius:6px;pointer-events:none;z-index:99999;box-sizing:border-box`;
          document.body.appendChild(d); return els.length;
        }, job.boxAround);
        console.log(`[${job.name}] boxAround -> ${n}`);
      }
      await page.screenshot({ path: `${job.out || OUT}/${job.name}.png`, fullPage: !!job.fullPage });
      console.log(`[${job.name}] saved  title="${await page.title()}"  url=${page.url()}`);
    } catch (e) {
      console.log(`[${job.name}] ERROR ${e.message}`);
    } finally { await page.close(); }
  }
  await browser.close();
})();
