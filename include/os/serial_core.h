#ifndef __OS_SERIAL_CORE_H
#define __OS_SERIAL_CORE_H

#include <os/spinlock.h>
#include <os/types.h>
#include <os/tty.h>

struct uart_driver;
struct uart_port;

struct uart_state {
    struct uart_port *port;
    struct tty_struct tty;
};

/** 从linux内核搬来的
 * 结构体 uart_ops —— serial_core 串口核心层与底层驱动之间的操作接口
 *
 * 本结构体定义了物理串口硬件支持的全部操作接口函数。
 *
 * @tx_empty: ``unsigned int ()(struct uart_port *port)``
 *
 *  该函数检测指定串口端口port的发送FIFO与移位寄存器是否为空。
 *  若发送通路完全为空，返回宏 %TIOCSER_TEMT；否则返回0。
 *  若硬件不支持该检测功能，也统一返回 %TIOCSER_TEMT。
 *
 *  锁保护：无锁。
 *  中断状态：由调用方决定是否关中断。
 *  本接口禁止睡眠阻塞
 *
 * @set_mctrl: ``void ()(struct uart_port *port, unsigned int mctrl)``
 *
 *  根据参数mctrl描述的状态，设置端口port的调制解调器控制线电平。
 *  mctrl中有效控制位定义如下：
 *
 *      - %TIOCM_RTS    RTS 请求发送信号
 *      - %TIOCM_DTR    DTR 数据终端就绪信号
 *      - %TIOCM_OUT1   自定义输出1信号
 *      - %TIOCM_OUT2   自定义输出2信号
 *      - %TIOCM_LOOP   开启端口硬件环回模式
 *
 *  对应比特置1时，将对应信号线置为有效电平；比特清零时，置为无效电平。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @get_mctrl: ``unsigned int ()(struct uart_port *port)``
 *
 *  获取当前端口port的调制解调器输入信号线实时状态。
 *  无需返回输出控制线状态，串口核心层会自行维护输出状态。
 *  返回值需包含以下输入信号状态位：
 *
 *      - %TIOCM_CAR    DCD 载波检测信号状态
 *      - %TIOCM_CTS    CTS 清除发送信号状态
 *      - %TIOCM_DSR    DSR 数据设备就绪信号状态
 *      - %TIOCM_RI     RI 振铃指示信号状态
 *
 *  信号线为有效电平时对应比特置1。
 *  若硬件无CTS/DCD/DSR引脚，驱动需恒标记该信号为有效；
 *  若无RI引脚，则不得标记RI为有效。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @stop_tx: ``void ()(struct uart_port *port)``
 *
 *  停止串口数据发送。触发场景：CTS信号失效、TTY上层下发XOFF流控字符要求暂停发送。
 *  驱动需立刻停止硬件发送动作。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @start_tx: ``void ()(struct uart_port *port)``
 *
 *  恢复串口数据发送。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @throttle: ``void ()(struct uart_port *port)``
 *
 *  通知串口驱动：线路规程层输入缓冲区即将满，需阻断外部继续向串口发送数据。
 *  仅开启硬件流控时才会调用本接口。
 *
 *  锁保护：与unthrottle、TTY层termios配置修改串行互斥执行
 *
 * @unthrottle: ``void ()(struct uart_port *port)``
 *
 *  通知串口驱动：线路规程输入缓冲区已腾出空间，允许外部继续发数据至串口，不会造成输入溢出。
 *  仅开启硬件流控时才会调用本接口。
 *
 *  锁保护：与throttle、TTY层termios配置修改串行互斥执行
 *
 * @send_xchar: ``void ()(struct uart_port *port, char ch)``
 *
 *  发送高优先级特殊字符，即使串口处于停止发送状态也要优先输出。
 *  用于实现XON/XOFF软件流控与tcflow系统调用。
 *  若驱动未实现该函数，TTY核心层会将字符追加至环形发送缓冲区，再调用start_tx/stop_tx刷出数据。
 *
 *  若 ch == '\0'（宏%__DISABLED_CHAR），则不执行发送操作。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @start_rx: ``void ()(struct uart_port *port)``
 *
 *  开启串口硬件接收通路。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @stop_rx: ``void ()(struct uart_port *port)``
 *
 *  关闭串口硬件接收通路；通常在串口端口执行关闭流程时调用。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @enable_ms: ``void ()(struct uart_port *port)``
 *
 *  使能调制解调器状态变更中断。
 *
 *  本函数可能被多次调用；shutdown接口执行时需关闭调制解调器状态中断。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @break_ctl: ``void ()(struct uart_port *port, int ctl)``
 *
 *  控制串口发送Break中断信号。ctl非0时持续输出Break电平；
 *  再次传入ctl=0时停止Break信号输出。
 *
 *  锁保护：调用方持有 tty_port->mutex 互斥锁
 *
 * @startup: ``int ()(struct uart_port *port)``
 *
 *  申请中断资源、初始化底层硬件状态，开启硬件接收通路。
 *  本函数不操作RTS/DTR电平，RTS/DTR由独立set_mctrl接口配置。
 *
 *  仅串口首次打开时调用一次。
 *
 *  锁保护：已持有 port_sem 信号量
 *  中断状态：全局中断关闭
 *
 * @shutdown: ``void ()(struct uart_port *port)``
 *
 *  关闭串口硬件、清除正在输出的Break信号、释放中断资源。
 *  不主动拉低RTS/DTR，上层会提前调用set_mctrl完成控制线复位。
 *
 *  本函数执行完成后，驱动严禁访问 port->state 成员。
 *  仅串口无任何用户占用时调用。
 *
 *  锁保护：已持有 port_sem 信号量
 *  中断状态：由调用方决定是否关中断
 *
 * @flush_buffer: ``void ()(struct uart_port *port)``
 *
 *  清空所有硬件发送缓存、重置DMA状态、终止正在进行的DMA传输。
 *  当 port->state->xmit 发送环形缓冲区清空时会触发调用。
 *
 *  锁保护：已持有 port->lock 自旋锁
 *  中断状态：本地中断已关闭
 *  本接口禁止睡眠阻塞
 *
 * @set_termios: ``void ()(struct uart_port *port, struct ktermios *new,
 *              struct ktermios *old)``
 *
 *  修改串口硬件参数：数据位宽、校验方式、停止位等。
 *  更新 port->read_status_mask / port->ignore_status_mask，配置需要上报给上层的硬件异常事件。
 *  ktermios.c_cflag 控制标志位说明：
 *
 *  - %CSIZE - 数据位长度掩码
 *  - %CSTOPB - 使用2位停止位
 *  - %PARENB - 使能奇偶校验
 *  - %PARODD - 奇校验（PARENB置位时生效）
 *  - %ADDRB - RS485地址标识位，通过uart_port.rs485_config配置
 *  - %CREAD - 开启接收；未置位时硬件仍接收数据，但驱动直接丢弃
 *  - %CRTSCTS - 置位则上报CTS电平变化中断
 *  - %CLOCAL - 未置位则上报调制解调器输入信号变化中断
 *
 *  ktermios.c_iflag 输入处理标志位说明：
 *
 *  - %INPCK - 帧错误、校验错误事件上报至TTY层
 *  - %BRKINT / %PARMRK - 两种标志均会将Break中断事件上报TTY层
 *  - %IGNPAR - 忽略校验错误、帧错误
 *  - %IGNBRK - 忽略Break信号；同时置IGNPAR时一并忽略溢出错误
 *
 *  c_iflag标志组合行为（以校验错误为例）：
 *
 *  ============ ======= ======= =========================================
 *  校验错误     INPCK   IGNPAR  行为说明
 *  ============ ======= ======= =========================================
 *  无错误        0       任意    正常字符，标记为%TTY_NORMAL
 *  有错误        1       0       上报字符并标记%TTY_PARITY校验错
 *  有错误        1       1       直接丢弃该字符
 *  ============ ======= ======= =========================================
 *
 *  若硬件支持软件流控，可使用XON/XOFF相关标志位。
 *
 *  锁保护：调用方持有 tty_port->mutex 互斥锁
 *  中断状态：由调用方决定是否关中断
 *  本接口禁止睡眠阻塞
 *
 * @set_ldisc: ``void ()(struct uart_port *port, struct ktermios *termios)``
 *
 *  线路规程切换通知回调，详情参考文档 Documentation/driver-api/tty/tty_ldisc.rst。
 *
 *  锁保护：调用方持有 tty_port->mutex 互斥锁
 *
 * @pm: ``void ()(struct uart_port *port, unsigned int state,
 *           unsigned int oldstate)``
 *
 *  执行串口端口相关电源管理操作。
 *  state：新电源状态（枚举uart_pm_state定义）；oldstate：切换前旧状态。
 *
 *  本函数不允许申请任何硬件资源。
 *
 *  串口打开/关闭时均会调用；若该串口同时作为系统控制台则跳过。
 *  即使未开启%CONFIG_PM电源管理配置，本接口仍会执行。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @type: ``const char *()(struct uart_port *port)``
 *
 *  返回描述当前串口硬件类型的静态字符串指针；返回NULL时内核自动替换为"unknown"。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @release_port: ``void ()(struct uart_port *port)``
 *
 *  释放端口当前占用的内存、IO地址区间资源。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @request_port: ``int ()(struct uart_port *port)``
 *
 *  申请串口所需内存、IO地址资源。申请失败时不注册任何资源，返回-%EBUSY。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @config_port: ``void ()(struct uart_port *port, int type)``
 *
 *  执行串口自动硬件识别配置，type为配置需求掩码：
 *  %UART_CONFIG_TYPE：探测识别串口硬件型号，检测成功则填充port->type，无设备置为%PORT_UNKNOWN。
 *  %UART_CONFIG_IRQ：自动探测中断号，使用内核标准自动探测接口；
 *  片上SOC硬件中断固定绑定的平台无需实现该探测。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @verify_port: ``int ()(struct uart_port *port,
 *               struct serial_struct *serinfo)``
 *
 *  校验传入的serinfo串口配置信息是否适配当前硬件类型。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @ioctl: ``int ()(struct uart_port *port, unsigned int cmd,
 *        unsigned long arg)``
 *
 *  处理串口硬件私有ioctl控制命令。命令号遵循<asm/ioctl.h>标准ioctl编号规范。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *
 * @poll_init: ``int ()(struct uart_port *port)``
 *
 *  KGDB内核调试器调用，完成极简硬件初始化，仅支撑poll_put_char/poll_get_char调试收发接口。
 *  与startup区别：不申请硬件中断资源。
 *
 *  锁保护：已持有全局tty_mutex与tty_port->mutex
 *  中断状态：无限制
 *
 * @poll_put_char: ``void ()(struct uart_port *port, unsigned char ch)``
 *
 *  KGDB调试器调用，直接向串口硬件输出单个字符ch；需阻塞等待TX FIFO有空位再写入。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *  本接口禁止睡眠阻塞
 *
 * @poll_get_char: ``int ()(struct uart_port *port)``
 *
 *  KGDB调试器调用，直接从串口硬件读取单个字符。有数据则返回字符；无数据立刻返回%NO_POLL_CHAR。
 *
 *  锁保护：无锁
 *  中断状态：由调用方决定是否关中断
 *  本接口禁止睡眠阻塞
 */
struct uart_ops {
	unsigned int	(*tx_empty)(struct uart_port *);
	void		(*set_mctrl)(struct uart_port *, unsigned int mctrl);
	unsigned int	(*get_mctrl)(struct uart_port *);
	void		(*stop_tx)(struct uart_port *);
	void		(*start_tx)(struct uart_port *);
	void		(*throttle)(struct uart_port *);
	void		(*unthrottle)(struct uart_port *);
	void		(*send_xchar)(struct uart_port *, char ch);
	void		(*stop_rx)(struct uart_port *);
	void		(*start_rx)(struct uart_port *);
	void		(*enable_ms)(struct uart_port *);
	void		(*break_ctl)(struct uart_port *, int ctl);
	int		(*startup)(struct uart_port *);
	void		(*shutdown)(struct uart_port *);
	void		(*flush_buffer)(struct uart_port *);
	void		(*set_termios)(struct uart_port *, struct ktermios *new,
				       const struct ktermios *old);
	void		(*set_ldisc)(struct uart_port *, struct ktermios *);
	void		(*pm)(struct uart_port *, unsigned int state,
			      unsigned int oldstate);
	const char	*(*type)(struct uart_port *);
	void		(*release_port)(struct uart_port *);
	int		(*request_port)(struct uart_port *);
	void		(*config_port)(struct uart_port *, int);
	int		(*ioctl)(struct uart_port *, unsigned int, unsigned long);
};


struct uart_port {
    spinlock_t lock;

    unsigned int line;       // 0 -> ttyS0
    unsigned int irq;
    unsigned long mapbase;
    void *membase;
    unsigned char		regshift;		/* reg offset shift */

    u32			(*serial_in)(struct uart_port *, unsigned int offset);
	void		(*serial_out)(struct uart_port *, unsigned int offset, u32 val);
    void			(*set_termios)(struct uart_port *,
				               struct ktermios *new,
				               const struct ktermios *old);
	void			(*set_ldisc)(struct uart_port *,
					     struct ktermios *);
    unsigned int		(*get_divisor)(struct uart_port *,
					       unsigned int baud,
					       unsigned int *frac);
	void			(*set_divisor)(struct uart_port *,
					       unsigned int baud,
					       unsigned int quot,
					       unsigned int quot_frac);
    int			(*startup)(struct uart_port *port);
	void			(*shutdown)(struct uart_port *port);
    int			(*handle_irq)(struct uart_port *);


    struct uart_driver *uart_driver;
    struct tty_port tty_port;

    void *private_data;
};


struct uart_driver {
    const char *driver_name;   // "qemu_uart"
    const char *dev_name;      // "ttyS"
    unsigned int major;
    unsigned int minor;
    unsigned int nr;           // 最大端口数
    struct tty_driver *tty_driver;
    struct uart_state *state;
};

int uart_register_driver(struct uart_driver *driver);
void uart_unregister_driver(struct uart_driver *driver);
int uart_add_one_port(struct uart_driver *driver, struct uart_port *port);
void uart_remove_one_port(struct uart_driver *driver, struct uart_port *port);

void uart_port_init(struct uart_port *port);
void uart_write_char(struct uart_port *port, char ch);
ssize_t uart_write(struct uart_port *port, const char *buf, size_t size);
void uart_receive_char(struct uart_port *port, char ch);
struct tty_struct *uart_port_tty(struct uart_port *port);

#endif
