package org.xhystudy.rpc.registry;

import org.xhystudy.rpc.spi.ExtensionLoader;

/**
注册工厂
 */
public class RegistryFactory {

    public static RegistryService get(String registryService) throws Exception {
        return ExtensionLoader.getInstance().get(registryService);
    }

    public static void init() throws Exception {
        ExtensionLoader.getInstance().loadExtension(RegistryService.class);
    }

}
