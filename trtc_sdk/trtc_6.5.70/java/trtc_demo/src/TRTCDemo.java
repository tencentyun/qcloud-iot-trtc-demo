import java.lang.System;
import com.engine.*;

public class TRTCDemo {
    private static EngineWarpper  _engine;

    /*
    * Demo 演示 房间组件的使用，进房10分钟后自动退出。
    * 获取并保存房间中其他用户 PCM 音频。
     */
    public static void main(String[] args){
        EngineWarpper.init();

        _engine = new EngineWarpper();
        _engine.start();
        try {
            Thread.sleep(600000);
        }catch (Exception e){

        }

        _engine.stop();

        System.out.println("finished!");
    }
}
