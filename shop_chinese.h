#ifndef _SHOP_CHINESE_H
#define _SHOP_CHINESE_H

/*
 * UTF-8 预编码中文字符串
 * 用于 ARMCC5 编译器 (源文件为 GBK/ANSI 编码时使用)
 * LVGL 文本渲染需要 UTF-8，但 ARMCC5 不支持 UTF-8 源文件
 * 因此这里用 \x 转义序列直接写入 UTF-8 字节
 */

// 商品名称
#define CN_APPLE      "\xe8\x8b\xb9\xe6\x9e\x9c"            // 苹果
#define CN_MILK       "\xe7\x89\x9b\xe5\xa5\xb6"            // 牛奶
#define CN_BREAD      "\xe9\x9d\xa2\xe5\x8c\x85"            // 面包
#define CN_WATERMELON "\xe8\xa5\xbf\xe7\x93\x9c"            // 西瓜
#define CN_COLA       "\xe5\x8f\xaf\xe4\xb9\x90"            // 可乐

// 单位
#define CN_BOX        "\xe7\x9b\x92"                        // 盒
#define CN_PACK       "\xe5\x8c\x85"                        // 包
#define CN_BOTTLE     "\xe7\x93\xb6"                        // 瓶
#define CN_YUAN       "\xe5\x85\x83"                        // 元

// 界面文字
#define CN_CART_TITLE "\xe8\xb4\xad\xe7\x89\xa9\xe8\xbd\xa6 (\xe7\x82\xb9\xe5\x87\xbb\xe5\x88\xa0\xe9\x99\xa4)"  // 购物车 (点击删除)
#define CN_CHECKOUT   "\xe7\xbb\x93\xe8\xb4\xa6"            // 结账
#define CN_CLEAR      "\xe6\xb8\x85\xe7\xa9\xba"            // 清空
#define CN_DISCOUNTS  "\xe6\x8a\x98\xe6\x89\xa3\xe6\xb4\xbb\xe5\x8a\xa8"  // 折扣活动
#define CN_NO_DISC    "\xe6\x97\xa0\xe4\xbc\x98\xe6\x83\xa0" // 无优惠
#define CN_FULL_RED   "\xe6\xbb\xa1" "20" "\xe5\x87\x8f" "5" // 满20减5
#define CN_90PCT      "9" "\xe6\x8a\x98"                     // 9折

// 动态提示字符串的中文部分（用于 snprintf 拼接）
#define CN_QTY_PREFIX "\xe8\xaf\xb7\xe8\xbe\x93\xe5\x85\xa5 ["  // 请输入 [
#define CN_QTY_MID    "] ("                                    // ] (
#define CN_QTY_SUFFIX ") \xe7\x9a\x84\xe6\x95\xb0\xe9\x87\x8f:"     // ) 的数量:

// 弹窗/提示文字
#define CN_OK         "\xe7\xa1\xae\xe5\xae\x9a"              // 确定
#define CN_CANCEL     "\xe5\x8f\x96\xe6\xb6\x88"              // 取消
#define CN_CLOSE      "\xe5\x85\xb3\xe9\x97\xad"              // 关闭
#define CN_ERROR      "\xe9\x94\x99\xe8\xaf\xaf"              // 错误
#define CN_HINT       "\xe6\x8f\x90\xe7\xa4\xba"              // 提示
#define CN_SUCCESS    "\xe6\x88\x90\xe5\x8a\x9f"              // 成功
#define CN_CART_EMPTY "\xe8\xb4\xad\xe7\x89\xa9\xe8\xbd\xa6\xe4\xb8\xba\xe7\xa9\xba" "!"   // 购物车为空!
#define CN_INT_ONLY   "\xe6\xad\xa4\xe5\x95\x86\xe5\x93\x81\xe5\xbf\x85\xe9\xa1\xbb\xe4\xb8\xba\xe6\x95\xb4\xe6\x95\xb0\xe6\x95\xb0\xe9\x87\x8f"  // 此商品必须为整数数量

// 结算结果文字
#define CN_BEFORE_DISC "\xe6\x8a\x98\xe6\x89\xa3\xe5\x89\x8d"  // 折扣前
#define CN_AFTER_DISC  "\xe6\x8a\x98\xe5\x90\x8e"             // 折后
#define CN_TOTAL       "\xe6\x80\xbb\xe8\xae\xa1"             // 总计
#define CN_DESC_FULL   " (\xe6\xbb\xa1" "20" "\xe5\x87\x8f" "5)"  //  (满20减5)
#define CN_DESC_90PCT  " (9" "\xe6\x8a\x98" ")"               //  (9折)
#define CN_DESC_NONE   ""                                     // (无折扣时不显示)

// 交易记录界面
#define CN_HISTORY     "\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // 交易记录
#define CN_HISTORY_TITLE "\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // 交易记录（标题）
#define CN_TX_NO       "\xe7\xac\xac"                         // 第
#define CN_TX_ID       "#\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"                        // #交易记录
#define CN_TX_ITEMS    "\xe9\xa1\xb9\xe5\x95\x86\xe5\x93\x81"  // 项商品
#define CN_TX_TOTAL    "\xe6\x80\xbb\xe8\xae\xa1"              // 总计
#define CN_TX_PAY      "\xe5\xae\x9e\xe4\xbb\x98"              // 实付
#define CN_TX_DISC     "\xe4\xbc\x98\xe6\x83\xa0"              // 优惠
#define CN_TX_EMPTY    "\xe6\x9a\x82\xe6\x97\xa0\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // 暂无交易记录
#define CN_TX_DETAIL   "\xe4\xba\xa4\xe6\x98\x93\xe6\x98\x8e\xe7\xbb\x86"  // 交易明细
#define CN_PAGE_PREV   "\xe4\xb8\x8a\xe4\xb8\x80\xe9\xa1\xb5"  // 上一页
#define CN_PAGE_NEXT   "\xe4\xb8\x8b\xe4\xb8\x80\xe9\xa1\xb5"  // 下一页
#define CN_HISTORY_CLR "\xe6\xb8\x85\xe7\xa9\xba\xe8\xae\xb0\xe5\xbd\x95"  // 清空记录
#define CN_PAGE_FMT    "\xe7\xac\xac %d/%d \xe9\xa1\xb5"      // 第 %d/%d 页
#define CN_CLOSE_BTN   LV_SYMBOL_CLOSE " " "\xe5\x85\xb3\xe9\x97\xad"     // ✕ 关闭
#define CN_QTY_X       "x"                                     // 数量分隔符
#define CN_TX_DISC_FMT "\xe4\xbc\x98\xe6\x83\xa0: %s"          // 优惠: %s

#endif
