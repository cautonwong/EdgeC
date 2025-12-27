// alarm_policy.js
// 使用 var 允许在持久上下文中反复执行而不会触发 redeclaration 错误

var v = DataHub.read(1001); 

if (v > 245) {
    Log.info("[ALARM] Overvoltage detected (" + v.toFixed(2) + "V)! Triggering emergency stop.");
    Arbiter.control(2001, 0); 
}