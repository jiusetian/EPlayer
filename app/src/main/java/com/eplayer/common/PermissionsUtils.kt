package com.eplayer.common

import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import android.support.v4.app.ActivityCompat
import android.support.v4.content.ContextCompat

/**
 * Author：Alex
 * Date：2019/10/9
 * Note：
 */
object PermissionsUtils {

    /**
     * 检测权限
     */
    fun checkPermission(context: Context, permission: String): Boolean =
        ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED

    /**
     * 返回未授权的权限
     */
    fun checkManyPermissions(context: Context, permissions: Array<String>): List<String> {
        return permissions.filter { !checkPermission(
            context,
            it
        )
        }
    }

    /**
     * 请求权限
     */
    fun requestPermission(context: Context, permission: String, requestCode: Int) {
        ActivityCompat.requestPermissions(context as Activity, arrayOf(permission), requestCode)
    }

    /**
     * 请求多个权限
     */
    fun requestManyPermissions(context: Context, permissions: Array<String>, requestCode: Int) {
        ActivityCompat.requestPermissions(context as Activity, permissions, requestCode)
    }

    /**
     * 判断是否已拒绝过权限
     *
     * @return
     * @describe :如果应用之前请求过此权限但用户拒绝，此方法将返回 true;
     * -----------如果应用第一次请求权限或 用户在过去拒绝了权限请求，
     * -----------并在权限请求系统对话框中选择了 Don't ask again 选项，此方法将返回 false。
     */
    fun judgePermission(context: Context, permission: String) =
        ActivityCompat.shouldShowRequestPermissionRationale(context as Activity, permission)


    /**
     * 检测权限并请求权限：如果没有权限，则请求权限
     */
    fun checkAndRequestPermission(context: Context, permission: String, requestCode: Int) {
        if (!checkPermission(context, permission))
            requestPermission(context, permission, requestCode)
    }

    /**
     * 检测并请求多个权限
     */
    fun checkAndRequestManyPermissions(context: Context, permissions: Array<String>, requestCode: Int) {
        val permissionList =
            checkManyPermissions(context, permissions)
        requestManyPermissions(
            context,
            permissionList.toTypedArray(),
            requestCode
        )
    }

    /**
     * 检测权限
     *
     * @describe：具体实现由回调接口决定
     */
    fun checkPermission(context: Context, permission: String, callBack: PermissionCheckCallBack) {
        if (checkPermission(context, permission)) { // 用户已授予权限
            callBack.onHavePermission()
        } else {
            if (judgePermission(context, permission)) { // 用户之前已拒绝过权限申请
                callBack.onUserHaveAlreadyDenied(arrayOf(permission))
            } else {  // 用户之前已拒绝并勾选了不在询问、用户第一次申请权限。
                callBack.onUserHaveAlreadyDeniedAndDontAsk(arrayOf(permission))
            }
        }
    }


    interface PermissionCheckCallBack {
        /**
         * 用户已经拥有权限
         */
        fun onHavePermission()

        /**
         * 用户已拒绝过权限
         *
         * @param permission:被拒绝的权限
         */
        fun onUserHaveAlreadyDenied(permissions: Array<String>)

        /**
         * 用户已拒绝过并且已勾选不再询问选项、用户第一次申请权限;
         *
         * @param permission:被拒绝的权限
         */
        fun onUserHaveAlreadyDeniedAndDontAsk(permissions: Array<String>)
    }
}





















