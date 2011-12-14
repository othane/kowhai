using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace kowhai_sharp
{
    using uint16_t = UInt16;
    using uint32_t = UInt32;
    using int16_t = Int16;
    using int32_t = Int32;

    public class Kowhai
    {
        public const string dllname = "kowhai_dll.dll";

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct kowhai_symbol_parts_t
        {
            public uint16_t name;
            public uint16_t array_index;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct kowhai_symbol_t
        {
            [FieldOffset(0)]
            public uint32_t symbol;
            [FieldOffset(0)]
            public kowhai_symbol_parts_t parts;
            public kowhai_symbol_t(uint32_t symbol)
            {
                this.parts.name = 0;
                this.parts.array_index = 0;
                this.symbol = symbol;
            }
            public kowhai_symbol_t(uint16_t name, uint16_t array_index)
            {
                this.symbol = 0;
                this.parts.name = name;
                this.parts.array_index = array_index;
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct kowhai_node_t
        {
            public uint16_t type;
            public uint16_t symbol;
            public uint16_t count;
            public uint16_t tag;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct kowhai_tree_t
        {
            [FieldOffset(0)]
            IntPtr desc;
            [FieldOffset(0)]
            IntPtr data;
        }

        public const int BRANCH = 0;
        public const int BRANCH_END = 1;

        public const int INT8 = 0x0070;
        public const int UINT8 = 0x0071;
        public const int INT16 = 0x0072;
        public const int UINT16 = 0x0073;
        public const int INT32 = 0x0074;
        public const int UINT32 = 0x0075;
        public const int FLOAT = 0x0076;
        public const int READONLY = 0x8000;

        public static int RawDataType(int dataType)
        {
            return dataType = dataType & (~READONLY);
        }

        public const int STATUS_OK = 0;
        public const int STATUS_INVALID_SYMBOL_PATH = 1;
        public const int STATUS_INVALID_DESCRIPTOR = 2;
        public const int STATUS_INVALID_OFFSET = 3;
        public const int STATUS_NODE_DATA_TOO_SMALL = 4;
        public const int STATUS_INVALID_NODE_TYPE = 5;
        public const int STATUS_PACKET_BUFFER_TOO_SMALL = 6;
        public const int STATUS_INVALID_PROTOCOL_COMMAND = 7;
        public const int STATUS_PACKET_BUFFER_TOO_BIG = 8;

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_node_type_size(uint16_t type);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_node(IntPtr tree_descriptor, int num_symbols, IntPtr symbols, out int offset, IntPtr target_node);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_node_size(IntPtr tree_descriptor, out int size);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_read(IntPtr tree, int num_symbols, IntPtr symbols, int read_offset, IntPtr result, int read_size);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_write(IntPtr tree, int num_symbols, IntPtr symbols, int write_offset, IntPtr value, int write_size);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_int8(IntPtr tree, int num_symbols, IntPtr symbols, out byte result);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_int16(IntPtr tree, int num_symbols, IntPtr symbols, out int16_t result);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_int32(IntPtr tree, int num_symbols, IntPtr symbols, out int32_t result);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_get_float(IntPtr tree, int num_symbols, IntPtr symbols, out float result);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_set_char(IntPtr tree, int num_symbols, IntPtr symbols, char value);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_set_int16(IntPtr tree, int num_symbols, IntPtr symbols, int16_t value);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_set_int32(IntPtr tree, int num_symbols, IntPtr symbols, int32_t value);

        [DllImport(dllname, CallingConvention = CallingConvention.Cdecl)]
        public static extern int kowhai_set_float(IntPtr tree, int num_symbols, IntPtr symbols, float value);

        public static int GetNode(kowhai_node_t[] descriptor, kowhai_symbol_t[] symbols, out int offset, out kowhai_node_t node)
        {
            GCHandle hDesc = GCHandle.Alloc(descriptor, GCHandleType.Pinned);
            GCHandle hSyms = GCHandle.Alloc(symbols, GCHandleType.Pinned);
            node = new kowhai_node_t();
            GCHandle hNode = GCHandle.Alloc(node, GCHandleType.Pinned);
            int result = kowhai_get_node(hDesc.AddrOfPinnedObject(), symbols.Length, hSyms.AddrOfPinnedObject(), out offset, hNode.AddrOfPinnedObject());
            hNode.Free();
            hSyms.Free();
            hDesc.Free();
            return result;
        }

        public static int GetNodeSize(kowhai_node_t[] descriptor, out int size)
        {
            GCHandle h = GCHandle.Alloc(descriptor, GCHandleType.Pinned);
            int result = kowhai_get_node_size(h.AddrOfPinnedObject(), out size);
            h.Free();
            return result;
        }

        public static kowhai_symbol_t[] GetSymbolPath(kowhai_node_t[] descriptor, kowhai_node_t node, int32_t nodeIndex,  uint16_t[] arrayIndexes)
        {
            Stack<kowhai_symbol_t> syms = new Stack<kowhai_symbol_t>();
            for (int i = 0; i <= nodeIndex; i++)
            {
                kowhai_node_t newNode = descriptor[i];
                if (i == nodeIndex)
                    syms.Push(new kowhai_symbol_t(newNode.symbol, arrayIndexes[syms.Count]));
                else if (newNode.type == BRANCH)
                    syms.Push(new kowhai_symbol_t(newNode.symbol, arrayIndexes[syms.Count]));
                else if (newNode.type == BRANCH_END)
                    syms.Pop();
            }
            kowhai_symbol_t[] result = syms.ToArray();
            Array.Reverse(result);
            return result;
        }
    }
}
