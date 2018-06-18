const React = require('react');
const assert = require('assert');
const Router = require('koa-router');
const send = require('koa-send');
const { renderToString } = require('react-dom/server');
const Html = require('../views/Html');
const admin = require('../views/admin.jsx');

console.log(Html);

const router = new Router({ prefix: '/admin' });

router.get('/', async (ctx, next) => {
  await send(ctx, 'public/html/admin.html');
});

router.get('/users', async (ctx, next) => {
  try {
    const result = await ctx.db.query('SELECT * FROM users;');
    assert.strictEqual(typeof (result), 'object', 'Result of query is not an object.');
    assert.strictEqual(Array.isArray(result.rows), true, 'Property \'rows\' of query result is not an array.');
    if (result.rows.length > 0) {
      ctx.status = 200;
      const body = renderToString(admin);
      const html = Html({body, title: 'Sample title'});
      ctx.body = html;
      // ctx.body = result.rows;
    } else {
      ctx.status = 404;
    }
  } catch (error) {
    console.log(error);
    ctx.throw();
  }
});

module.exports = router;
