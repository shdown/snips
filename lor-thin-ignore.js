// ==UserScript==
// @name        LOR thin ignore
// @namespace   http://localhost
// @include     /^https?://(www\.)?linux.org.ru/.*$/
// @run-at      document-start
// @version     1
// @grant       none
// ==/UserScript==
var ignored = new Set([
    // 'maxcom',
]);
function hide_them() {
  $('.msg-container').each(function(idx, msg) {
    var author = $('a[itemprop=creator]', msg).text();
    if (ignored.has(author)) {
      $(msg).html('(A post by <b>' + author + '</b>)');
    }
  });
}
var done = false;
function update() {
  if (document.body) {
    document.body.style.display = done ? '' : 'none';
  }
}
window.addEventListener('beforescriptexecute', update);
window.addEventListener('load', function() { hide_them(); done = true; update(); });
