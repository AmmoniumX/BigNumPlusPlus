using System;
using System.Globalization;

namespace BigNumLibrary
{
    // Context for formatting BigNum output
    public class BigNumContext
    {
        // Up to how many "real" digits to display before using scientific notation
        public uint MaxDigits { get; set; } = 10;
        // How many fractional digits to display in scientific notation
        public uint PrintPrecision { get; set; } = 2;

        public static BigNumContext Default { get; } = new BigNumContext();
    }

    // Helper class that provides a table of powers of 10
    public static class Pow10
    {
        // Using 308 as the typical max_exponent10 for double
        public const int Pow10TableOffset = 308;
        public const int Pow10TableSize = 2 * Pow10TableOffset + 1;
        public static readonly double[] Pow10Table;

        static Pow10()
        {
            Pow10Table = new double[Pow10TableSize];
            Pow10Table[Pow10TableOffset] = 1.0;
            double pos = 1.0;
            for (int i = 1; i <= Pow10TableOffset; i++)
            {
                pos *= 10.0;
                Pow10Table[Pow10TableOffset + i] = pos;      // positive exponents: 10^i
                Pow10Table[Pow10TableOffset - i] = 1.0 / pos;  // negative exponents: 10^(-i)
            }
        }

        // Returns the 10^e value if within range; otherwise, null.
        public static double? Get(int e)
        {
            if (e < -Pow10TableOffset || e > Pow10TableOffset)
                return null;
            return Pow10Table[e + Pow10TableOffset];
        }
    }

    // The BigNum class stores numbers as a normalized pair: m * 10^e, with m in [1,10)
    public class BigNum : IComparable<BigNum>
    {
        private double m; // mantissa
        private ulong e;  // exponent

        // Private constructor that allows turning normalization on or off.
        private BigNum(double mantissa, ulong exponent, bool normalize)
        {
            m = mantissa;
            e = exponent;
            if (normalize)
                Normalize();
        }

        // Public constructors
        public BigNum(double mantissa, ulong exponent = 0)
            : this(mantissa, exponent, true)
        {
        }

        public BigNum(string str)
        {
            ParseStr(str);
        }

        public BigNum(BigNum other)
        {
            m = other.m;
            e = other.e;
        }

        public BigNum(long value)
            : this((double)value, 0)
        {
        }

        // Parsing strings of the form "mantissaeexponent", e.g. "3.14e2"
        private void ParseStr(string str)
        {
            try
            {
                int pos = str.IndexOf('e');
                if (pos != -1)
                {
                    string mantissaPart = str.Substring(0, pos);
                    string exponentPart = str.Substring(pos + 1);
                    m = double.Parse(mantissaPart, CultureInfo.InvariantCulture);
                    e = ulong.Parse(exponentPart, CultureInfo.InvariantCulture);
                }
                else
                {
                    m = double.Parse(str, CultureInfo.InvariantCulture);
                    e = 0;
                }
                Normalize();
            }
            catch (Exception ex)
            {
                throw new ArgumentException("Failed to parse number: " + ex.Message);
            }
        }

        // Normalize the number so that m is in [1,10). Numbers less than 1 (with e == 0) are set to 0.
        public void Normalize()
        {
            // If this is one of our special values, leave it as is.
            if (this.Equals(Max) || this.Equals(Min))
                return;
            if (double.IsNaN(m))
            {
                e = 0;
                return;
            }
            if (double.IsInfinity(m))
            {
                e = 0;
                return;
            }
            if (m == 0)
            {
                e = 0;
                return;
            }
            // Any number less than 1 with no exponent is considered 0.
            if (Math.Abs(m) < 1 && e == 0)
            {
                m = 0;
                return;
            }

            int n_log = (int)Math.Floor(Math.Log10(Math.Abs(m)));
            double? factor = Pow10.Get(n_log);
            if (factor == null)
                throw new Exception("Exponent out of range during normalization.");
            m = m / factor.Value;
            // Since e is unsigned, ensure that subtraction does not underflow.
            try
            {
                checked
                {
                    e = (ulong)((long)e + n_log);
                }
            }
            catch (OverflowException)
            {
                // Clamp to maximum value if overflow occurs.
                m = Max.m;
                e = Max.e;
                return;
            }

            if (Math.Abs(m) < 1 && e == 0)
            {
                m = 0;
                return;
            }

            if (this.CompareTo(Max) > 0)
                Set(Max);
            if (this.CompareTo(Min) < 0)
                Set(Min);
        }

        private void Set(BigNum other)
        {
            m = other.m;
            e = other.e;
        }

        // Public accessors for the internal representation.
        public double M => m;
        public ulong E => e;

        // Special constants
        public static BigNum Inf => new BigNum(double.PositiveInfinity, 0, false);
        public static BigNum NaN => new BigNum(double.NaN, 0, false);
        public static BigNum Max => new BigNum(Math.BitDecrement(10.0), ulong.MaxValue, false);
        public static BigNum Min => new BigNum(0.0, 0, false);

        // Arithmetic operations
        public BigNum Add(BigNum b)
        {
            bool thisIsBigger = this.e > b.e;
            ulong delta = thisIsBigger ? this.e - b.e : b.e - this.e;
            double m2;
            ulong e2;
            if (delta > 14)
            {
                if (thisIsBigger)
                {
                    m2 = this.m;
                    e2 = this.e;
                }
                else
                {
                    m2 = b.m;
                    e2 = b.e;
                }
            }
            else if (thisIsBigger)
            {
                double? factor = Pow10.Get((int)delta);
                if (factor == null)
                    throw new Exception("Exponent difference out of range in addition.");
                m2 = this.m * factor.Value + b.m;
                e2 = b.e;
            }
            else
            {
                double? factor = Pow10.Get((int)delta);
                if (factor == null)
                    throw new Exception("Exponent difference out of range in addition.");
                m2 = this.m + b.m * factor.Value;
                e2 = this.e;
            }
            return new BigNum(m2, e2);
        }

        public BigNum Sub(BigNum b)
        {
            return Add(new BigNum(-b.m, b.e));
        }

        public BigNum Mul(BigNum b)
        {
            return new BigNum(this.m * b.m, this.e + b.e);
        }

        public BigNum Div(BigNum b)
        {
            if (b.m == 0)
                return NaN;
            if (b.e > this.e)
                return new BigNum(0.0, 0);
            if (b.e == this.e && Math.Abs(this.m / b.m) < 1)
                return new BigNum(0.0, 0);
            return new BigNum(this.m / b.m, this.e - b.e);
        }

        public BigNum Abs()
        {
            return new BigNum(Math.Abs(m), e);
        }

        public BigNum Negate()
        {
            return this.Mul(new BigNum(-1.0, 0));
        }

        // Operator overloads for convenience.
        public static BigNum operator +(BigNum a, BigNum b) => a.Add(b);
        public static BigNum operator -(BigNum a, BigNum b) => a.Sub(b);
        public static BigNum operator *(BigNum a, BigNum b) => a.Mul(b);
        public static BigNum operator /(BigNum a, BigNum b) => a.Div(b);
        public static BigNum operator -(BigNum a) => a.Negate();

        // Comparison implementation
        public int CompareTo(BigNum b)
        {
            if (this.m == b.m && this.e == b.e)
                return 0;
            if (this.IsPositive() && b.IsNegative())
                return 1;
            if (this.IsNegative() && b.IsPositive())
                return -1;
            if (this.e > b.e)
                return 1;
            if (this.e < b.e)
                return -1;
            if (this.IsPositive() && this.m > b.m)
                return 1;
            if (this.IsPositive() && this.m < b.m)
                return -1;
            if (this.IsNegative() && this.m > b.m)
                return -1;
            if (this.IsNegative() && this.m < b.m)
                return 1;
            return 0;
        }

        public static bool operator <(BigNum a, BigNum b) => a.CompareTo(b) < 0;
        public static bool operator >(BigNum a, BigNum b) => a.CompareTo(b) > 0;
        public static bool operator <=(BigNum a, BigNum b) => a.CompareTo(b) <= 0;
        public static bool operator >=(BigNum a, BigNum b) => a.CompareTo(b) >= 0;
        public static bool operator ==(BigNum a, BigNum b) => a.CompareTo(b) == 0;
        public static bool operator !=(BigNum a, BigNum b) => a.CompareTo(b) != 0;

        public override bool Equals(object obj)
        {
            if (obj is BigNum bn)
                return this.CompareTo(bn) == 0;
            return false;
        }

        public override int GetHashCode()
        {
            return m.GetHashCode() ^ e.GetHashCode();
        }

        // Helper methods to test sign.
        public bool IsPositive() => m >= 0;
        public bool IsNegative() => m < 0;

        // Conversion to string – either as a full number (if it fits) or in scientific notation.
        public string ToString(uint precision = 0)
        {
            if (precision == 0)
                precision = BigNumContext.Default.PrintPrecision;

            if (double.IsInfinity(m))
                return "inf";
            if (double.IsNaN(m))
                return "nan";

            uint maxDigits = Math.Max(precision + 1, BigNumContext.Default.MaxDigits);
            if (e < maxDigits - 1)
            {
                // Try to display the number fully.
                string str = m.ToString(CultureInfo.InvariantCulture);
                int newLen = (int)(Math.Min((ulong)maxDigits, e + 1) + (str.StartsWith("-") ? 1 : 0));
                // Remove any decimal point.
                str = str.Replace(".", "");
                if (str.Length < newLen)
                    str = str.PadRight(newLen, '0');
                else if (str.Length > newLen)
                    str = str.Substring(0, newLen);
                return str;
            }

            // Otherwise, use scientific notation.
            string m_str = ToStringFloor(m, (int)precision);
            int dotIndex = m_str.IndexOf('.');
            if (dotIndex != -1 && dotIndex + (int)precision + 1 < m_str.Length)
            {
                m_str = m_str.Substring(0, dotIndex + (int)precision + 1);
            }
            return e != 0 ? m_str + "e" + e.ToString() : m_str;
        }

        // Helper method: returns a fixed-point string with the value floored to the given precision.
        public static string ToStringFloor(double value, int precision)
        {
            double scale = Math.Pow(10.0, precision);
            double truncated = Math.Floor(value * scale) / scale;
            return truncated.ToString("F" + precision, CultureInfo.InvariantCulture);
        }

        // Attempts to convert the BigNum to a long, returning null if it would overflow.
        public long? ToNumber()
        {
            double logVal = Math.Log10(Math.Abs(m));
            double totalDigits = e + logVal + 1;
            // If there are more than roughly 19 digits, the number is too large.
            if (totalDigits > 19)
                return null;
            double? factor = Pow10.Get((int)e);
            if (factor == null)
                return null;
            double result = m * factor.Value;
            if (result > long.MaxValue || result < long.MinValue)
                return null;
            return (long)result;
        }

        // Returns log10(this) if representable.
        public double? Log10()
        {
            double log_m = Math.Log10(m);
            if (double.MaxValue - e < log_m)
                return null;
            return e + log_m;
        }

        // Raises this BigNum to the given power.
        public BigNum Pow(long power)
        {
            if (power == 0)
                return new BigNum(1.0, 0);
            if (m == 0)
            {
                if (power < 0)
                    throw new ArgumentException("Cannot raise 0 to a negative power");
                return new BigNum(0.0, 0);
            }

            if (m < 0)
            {
                if (power % 2 == 0)
                    return new BigNum(-m, e, false).Pow(power);
                else
                    return new BigNum(-m, e, false).Pow(power).Negate();
            }

            double? logVal = this.Log10();
            if (logVal == null)
                return new BigNum(0.0, 0);
            bool isNegativePower = power < 0;
            long absPower = Math.Abs(power);
            double new_log = logVal.Value * absPower;
            if (isNegativePower)
                new_log = -new_log;
            // If the result would be smaller than what can be represented (i.e. in (0,1)), return 0.
            if (new_log < 0)
                return new BigNum(0.0, 0);
            double fractional = new_log - Math.Floor(new_log);
            double new_m = Math.Pow(10, fractional);
            ulong new_e = (ulong)Math.Floor(new_log);
            return new BigNum(new_m, new_e);
        }

        // Returns the nth root of this BigNum.
        public BigNum Root(long n)
        {
            if (n == 0)
                throw new ArgumentException("Cannot take the zeroth root");
            if (m == 0)
                return new BigNum(0.0, 0);
            bool isNegative = m < 0;
            if (isNegative && n % 2 == 0)
                throw new ArgumentException("Even root of a negative number is not defined");

            double absLog = Math.Log10(Math.Abs(m)) + e;
            double new_log = absLog / n;
            ulong new_e = (ulong)Math.Floor(new_log);
            double fractional = new_log - Math.Floor(new_log);
            double new_m = Math.Pow(10.0, fractional);
            if (isNegative)
                new_m = -new_m;
            return new BigNum(new_m, new_e);
        }

        // Convenience: square root.
        public BigNum Sqrt() => Root(2);
    }
}
