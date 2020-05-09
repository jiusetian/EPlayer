package com.eplayer

import android.Manifest
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.support.v7.app.AppCompatActivity
import com.eplayer.common.LogUtil
import com.eplayer.common.Utils
import kotlinx.android.synthetic.main.activity_main.*
import pub.devrel.easypermissions.AppSettingsDialog
import pub.devrel.easypermissions.EasyPermissions
import pub.devrel.easypermissions.PermissionRequest
import java.io.File

class MainActivity : AppCompatActivity(), EasyPermissions.PermissionCallbacks {

    companion object {
        private const val REQUEST_PERMISSION_CODE = 10
        private val perms = arrayOf(
            Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO, Manifest.permission.READ_EXTERNAL_STORAGE
            , Manifest.permission.WRITE_EXTERNAL_STORAGE
        ) //权限

    }

    //视频播放路径
     private val PATH = Environment.getExternalStorageDirectory().getPath() + File.separator + "hello.mp4"
    //private val PATH = "rtmp://58.200.131.2:1935/livetv/hunantv"
    //private val PATH = "http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8"



    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        checkPerms() //初始化权限
        btn_live.setOnClickListener {
            if (EasyPermissions.hasPermissions(this, *perms))
                startActivity(Intent(this, LiveActivity::class.java))
            else
                requestPerms(*perms)
        }

        LogUtil.d("返回媒体信息："+Utils.getVideoList(this).get(0))

        //播放器播放
        btn_play.setOnClickListener {
            val intent = Intent(this, MediaPlayerActivity::class.java)
            intent.putExtra("path", PATH)
            startActivity(intent)
        }

    }

    /**
     * 初始化权限相关
     */
    private fun checkPerms() {
        LogUtil.d("执行权限")

        //数组对象前加*号可以将数组展开，方便传值
        if (EasyPermissions.hasPermissions(this, *perms)) {
        } else { //没有权限，则申请
            requestPerms(*perms)
        }
    }

    //请求权限
    private fun requestPerms(vararg perms: String) {
        EasyPermissions.requestPermissions(
            PermissionRequest.Builder(this, REQUEST_PERMISSION_CODE, *perms)
                .setRationale("为了App正常使用，请允许必要的权限")
                .setPositiveButtonText("确定")
                .setNegativeButtonText("取消")
                .build()
            //this, "为了正常使用应用，请允许必要的权限！", REQUEST_PERMISSION_CODE, *perms
        )
    }


    //接受系统权限的处理，这里交给EasyPermissions来处理，回调到 EasyPermissions.PermissionCallbacks接口
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        //注意这个this，内部对实现该方法进行了查询，所以没有this的话，回调结果的方法不生效
        EasyPermissions.onRequestPermissionsResult(requestCode, permissions, grantResults, this)
    }

    /*
     * 权限允许与否的回调
     */
    override fun onPermissionsDenied(requestCode: Int, perms: MutableList<String>) {
        //如果用户点击永远禁止，这个时候就需要跳到系统设置页面去手动打开了
        if (EasyPermissions.somePermissionPermanentlyDenied(this, perms)) {
            AppSettingsDialog.Builder(this).build().show()
        }
    }

    //权限被允许了
    override fun onPermissionsGranted(requestCode: Int, perms: MutableList<String>) {

    }
}
