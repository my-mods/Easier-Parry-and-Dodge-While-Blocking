-- Settings contract shared by the loader and Mod Setting Menu. MIT License.
return {
    {key="enabled", default=1, values={0,1}},
    {key="parryWindowPercent", default=200, min=10, max=5000, integer=false},
    {key="dodgeWhileBlocking", default=1, values={0,1}},
    {key="dodgeWindowPercent", default=100, min=10, max=5000, integer=false},
    {key="debugLogging", default=0, values={0,1}},
}
