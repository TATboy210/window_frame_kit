import 'dart:async';

/// example 专用的限时鼠标穿透；串行平台调用避免迟到开启覆盖恢复。
/// 不改变插件 API，异常交由调用方显示；dispose 会尽力恢复鼠标交互。
final class MousePassthroughRecovery {
  MousePassthroughRecovery({
    required this.apply,
    required this.onError,
    Duration recoveryDelay = const Duration(seconds: 5),
  }) : _delay = recoveryDelay;

  /// 执行原生穿透状态写入。
  final Future<void> Function(bool) apply;

  /// 显示平台可恢复异常，不吞掉错误。
  final void Function(Exception, StackTrace) onError;
  final Duration _delay;
  Future<void> _queue = Future<void>.value();
  Timer? _timer;
  bool _disposed = false;
  bool _needsRestore = false;
  int _generation = 0;

  /// 开启后重新计时，关闭立即取消计时；所有原生写入按请求顺序执行。
  Future<void> setEnabled(bool enabled) {
    if (_disposed) return Future<void>.value();
    _timer?.cancel();
    final generation = ++_generation;
    final operation = _enqueue(enabled);
    return operation.then((_) {
      if (!enabled || _disposed || generation != _generation) return;
      // 即便开启报告异常也做兜底恢复，原生调用可能已部分生效。
      _timer = Timer(_delay, () => unawaited(setEnabled(false)));
    });
  }

  /// 队列同时覆盖成功和可恢复异常，后续关闭不会被前次平台错误阻断。
  Future<void> _enqueue(bool enabled) {
    _queue = _queue.then((_) async {
      if (enabled) _needsRestore = true;
      try {
        await apply(enabled);
        if (!enabled) _needsRestore = false;
      } on Exception catch (error, stack) {
        onError(error, stack);
      }
    });
    return _queue;
  }

  /// 先等已发起的开启结束，再补发恢复；销毁幂等且不再创建定时器。
  Future<void> dispose() {
    if (_disposed) return _queue;
    _disposed = true;
    ++_generation;
    _timer?.cancel();
    _queue = _queue.then((_) async {
      if (!_needsRestore) return;
      try {
        await apply(false);
        _needsRestore = false;
      } on Exception catch (error, stack) {
        onError(error, stack);
      }
    });
    return _queue;
  }
}
