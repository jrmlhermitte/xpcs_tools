import numpy as np
from matplotlib.pyplot import plot

def plcirc(rc, R,ex=1.,ey=1.,**kwargs):
    ''' Plot a circle.
        ex,ey : epsilonx, epsilony stretch factors
    '''
    pphi = np.linspace(0., np.pi*2,1000);
    plot(R*np.cos(pphi)*ex+rc[0],  R*np.sin(pphi)*ey + rc[1],hold=True,**kwargs)

def plcircs(rcs, Rs, **kwargs):
    for rc, R in zip(rcs, Rs):
        plcirc(rc, R,**kwargs)
