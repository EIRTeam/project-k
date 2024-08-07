def can_build(env, platform):
    if not env.debug_features:
        return False
    return True


def configure(env):
    pass
