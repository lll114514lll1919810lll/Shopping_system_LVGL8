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
#define CN_CHOCOLATE  "\xe5\xb7\xa7\xe5\x85\x8b\xe5\x8a\x9b"  // 巧克力

// 单位
#define CN_BOX        "\xe7\x9b\x92"                        // 盒
#define CN_PACK       "\xe5\x8c\x85"                        // 包
#define CN_BOTTLE     "\xe7\x93\xb6"                        // 瓶
#define CN_YUAN       "\xe5\x85\x83"                        // 元

// 界面文字
#define CN_HOME_TITLE  "\xe8\xb4\xad\xe7\x89\xa9\xe7\xb3\xbb\xe7\xbb\x9f"  // 购物系统
#define CN_BTN_SHOP    "\xe8\xb4\xad\xe7\x89\xa9"            // 购物
#define CN_BTN_HISTORY "\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // 交易记录
#define CN_BTN_BACK    "\xe8\xbf\x94\xe5\x9b\x9e\xe4\xb8\xbb\xe9\xa1\xb5"  // 返回主页
#define CN_CHECKOUT   "\xe7\xbb\x93\xe8\xb4\xa6"            // 结账
#define CN_CLEAR      "\xe6\xb8\x85\xe7\xa9\xba"            // 清空
#define CN_DISCOUNTS  "\xe6\x8a\x98\xe6\x89\xa3\xe6\xb4\xbb\xe5\x8a\xa8"  // 折扣活动
#define CN_DISC_NONE         "\xe6\x97\xa0\xe4\xbc\x98\xe6\x83\xa0"                        // 无优惠
#define CN_DISC_FULL20_RED5  "\xe6\xbb\xa1" "20" "\xe5\x87\x8f" "5"                         // 满20减5
#define CN_DISC_90PCT        "\xe6\x89\x93" "9" "\xe6\x8a\x98"                              // 打9折
#define CN_DISC_FULL100_80PCT "\xe6\xbb\xa1" "100" "\xe6\x89\x93" "8" "\xe6\x8a\x98"        // 满100打8折
#define CN_DISC_FULL200_RED50 "\xe6\xbb\xa1" "200" "\xe5\x87\x8f" "50"                      // 满200减50
#define CN_COUPON_REMAINING "%s (\xe5\x89\xa9 %d \xe5\xbc\xa0\xe5\x8f\xaf\xe7\x94\xa8)"	// %s (剩 %d 张可用)

// 动态提示字符串的中文部分（用于 snprintf 拼接）
#define CN_QTY_PREFIX "\xe8\xaf\xb7\xe8\xbe\x93\xe5\x85\xa5 ["  // 请输入 [
#define CN_QTY_MID    "] ("                                    // ] (
#define CN_QTY_SUFFIX ") \xe7\x9a\x84\xe6\x95\xb0\xe9\x87\x8f:"     // ) 的数量:

// 弹窗/提示文字
#define CN_OK         "\xe7\xa1\xae\xe5\xae\x9a"              // 确定
#define CN_ERROR      "\xe9\x94\x99\xe8\xaf\xaf"              // 错误
#define CN_HINT       "\xe6\x8f\x90\xe7\xa4\xba"              // 提示
#define CN_SUCCESS    "\xe6\x88\x90\xe5\x8a\x9f"              // 成功
#define CN_CART_EMPTY "\xe8\xb4\xad\xe7\x89\xa9\xe8\xbd\xa6\xe4\xb8\xba\xe7\xa9\xba" "!"   // 购物车为空!
#define CN_INT_ONLY   "\xe6\xad\xa4\xe5\x95\x86\xe5\x93\x81\xe5\xbf\x85\xe9\xa1\xbb\xe4\xb8\xba\xe6\x95\xb4\xe6\x95\xb0\xe6\x95\xb0\xe9\x87\x8f"  // 此商品必须为整数数量
#define CN_QTY_POSITIVE "\xe6\x95\xb0\xe9\x87\x8f\xe5\xbf\x85\xe9\xa1\xbb\xe5\xa4\xa7\xe4\xba\x8e" "0"  // 数量必须大于0

// 购物车操作
#define CN_CART_TITLE "\xe8\xb4\xad\xe7\x89\xa9\xe8\xbd\xa6 (\xe7\x82\xb9\xe5\x87\xbb\xe4\xbf\xae\xe6\x94\xb9)"  // 购物车 (点击修改)
#define CN_CART_ACTION "\xe6\x93\x8d\xe4\xbd\x9c\xe9\x80\x89\xe6\x8b\xa9"              // 操作选择
#define CN_DELETE_ITEM "\xe5\x88\xa0\xe9\x99\xa4\xe5\x95\x86\xe5\x93\x81"            // 删除商品
#define CN_EDIT_QTY    "\xe4\xbf\xae\xe6\x94\xb9\xe6\x95\xb0\xe9\x87\x8f"            // 修改数量
#define CN_NEW_QTY_PREFIX "\xe8\xaf\xb7\xe8\xbe\x93\xe5\x85\xa5 ["                   // 请输入 [
#define CN_NEW_QTY_MID  "] \xe7\x9a\x84\xe6\x95\xb0\xe9\x87\x8f:"       // ] 的数量:

// 结算结果文字
#define CN_BEFORE_DISC "\xe6\x8a\x98\xe6\x89\xa3\xe5\x89\x8d"  // 折扣前
#define CN_AFTER_DISC  "\xe6\x8a\x98\xe5\x90\x8e"             // 折后
#define CN_TOTAL       "\xe6\x80\xbb\xe8\xae\xa1"             // 总计
#define CN_DESC_DISC_FULL20        " (\xe6\xbb\xa1" "20" "\xe5\x87\x8f" "5)"                       //  (满20减5)
#define CN_DESC_DISC_90PCT         " (9" "\xe6\x8a\x98" ")"                                        //  (9折)
#define CN_DESC_DISC_FULL100_80PCT " (\xe6\xbb\xa1" "100" "\xe6\x89\x93" "8" "\xe6\x8a\x98" ")"    //  (满100打8折)
#define CN_DESC_DISC_FULL200_RED50 " (\xe6\xbb\xa1" "200" "\xe5\x87\x8f" "50" ")"                  //  (满200减50)
#define CN_DESC_DISC_NONE          ""                                                              // // (无折扣时不显示)

// 交易记录界面
#define CN_HISTORY     "\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // 交易记录
#define CN_TX_ID       "#\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // #交易记录
#define CN_TX_ITEMS    "\xe9\xa1\xb9\xe5\x95\x86\xe5\x93\x81"  // 项商品
#define CN_TX_TOTAL    "\xe6\x80\xbb\xe8\xae\xa1"              // 总计
#define CN_TX_PAY      "\xe5\xae\x9e\xe4\xbb\x98"              // 实付
#define CN_TX_EMPTY    "\xe6\x9a\x82\xe6\x97\xa0\xe4\xba\xa4\xe6\x98\x93\xe8\xae\xb0\xe5\xbd\x95"  // 暂无交易记录
#define CN_TX_DETAIL   "\xe4\xba\xa4\xe6\x98\x93\xe6\x98\x8e\xe7\xbb\x86"  // 交易明细
#define CN_HISTORY_CLR "\xe6\xb8\x85\xe7\xa9\xba\xe8\xae\xb0\xe5\xbd\x95"  // 清空记录
#define CN_CLOSE_BTN   LV_SYMBOL_CLOSE " " "\xe5\x85\xb3\xe9\x97\xad"     // ✕ 关闭
#define CN_QTY_X       "x"                                     // 数量分隔符
#define CN_TX_DISC_FMT "\xe4\xbc\x98\xe6\x83\xa0: %s"          // 优惠: %s
#define CN_MAX_RECORDS "\xe6\x9c\x80\xe5\xa4\x9a\xe5\x82\xa8\xe5\xad\x98 %d \xe6\x9d\xa1\xe8\xae\xb0\xe5\xbd\x95\xef\xbc\x8c\xe7\x82\xb9\xe5\x87\xbb\xe6\x9f\xa5\xe7\x9c\x8b\xe8\xaf\xa6\xe6\x83\x85"  // 最多储存 %d 条记录，点击查看详情
#define CN_COUPON_USED_UP "\xe8\xaf\xa5\xe4\xbc\x98\xe6\x83\xa0\xe5\x88\xb8\xe5\xb7\xb2\xe7\x94\xa8\xe5\xae\x8c"  // 该优惠券已用完


#endif
