// computed.cjs NODE_MODULES CHROME [ARG...] < JOBS: what a browser computes.
// A job is {url, system, read}: the page, the system's colour scheme and the
// expression to evaluate there. The answers come out as a JSON list.
const path = require('path');
const fs = require('fs');

const [modules, chrome, ...args] = process.argv.slice(2);
const puppeteer = require(path.join(modules, 'puppeteer-core'));
const jobs = JSON.parse(fs.readFileSync(0, 'utf8'));

(async () => {
  const browser = await puppeteer.launch({executablePath: chrome, args});
  try {
    const page = await browser.newPage();
    const answers = [];
    for (const job of jobs) {
      await page.emulateMediaFeatures([{name: 'prefers-color-scheme', value: job.system}]);
      const response = await page.goto(job.url, {waitUntil: 'load'});
      answers.push({status: response ? response.status() : null, value: await page.evaluate(job.read)});
    }
    process.stdout.write(JSON.stringify(answers));
  } finally {
    await browser.close();
  }
})().catch((error) => {
  process.stderr.write(`${error.stack || error}\n`);
  process.exit(1);
});
